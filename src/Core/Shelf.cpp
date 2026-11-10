#include <Core/Shelf.h>
#include <Core/Log.h>
#include <Surfaces/Dock.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/XDG/XDGDesktopEntry.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

static Shelf *_shelf;

Shelf *GetShelf() noexcept { return _shelf; }

Shelf::Shelf() noexcept
{
    _shelf = this;

    MOptions options;
    options.extensions.add(MForeignToplevelManagerExt);

    marco = MApp::Make(options);
    if (!marco)
        return;

    xdgkit = XDGKit::Make();
    settings.load();
    assets.init();
    initWindows();

    marco->onScreenPlugged.subscribe(this, [this](MScreen &s){ onScreenPlugged(s); });
    marco->onScreenUnplugged.subscribe(this, [this](MScreen &s){ onScreenUnplugged(s); });

    for (MScreen *s : marco->screens())
        onScreenPlugged(*s);
}

Shelf::~Shelf() noexcept
{
    if (marco)
        for (MScreen *s : marco->screens())
            onScreenUnplugged(*s);

    _shelf = nullptr;
}

void Shelf::onScreenPlugged(MScreen &screen) noexcept
{
    if (screen.userData)
        return; // Already has a dock

    Log(CZInfo, CZLN, "Screen plugged: {}", screen.props().name);
    auto *dock { new Dock(screen) }; // Sets screen.userData

    // No settings yet: CZ_SHELF_AUTOHIDE=0 keeps the dock shown.
    if (const char *env { getenv("CZ_SHELF_AUTOHIDE") }; env && strcmp(env, "0") == 0)
        dock->setAutoHide(false);
}

void Shelf::onScreenUnplugged(MScreen &screen) noexcept
{
    auto *dock { static_cast<Dock*>(screen.userData) };
    screen.userData = nullptr;

    // The screen is still listed while this signal is emitted.
    const size_t remaining { marco->screens().size() - 1 };
    Log(CZInfo, CZLN, "Screen unplugged: {} ({} screen(s) left)", screen.props().name, remaining);

    delete dock;
}

/* ------------------------------------ Window model ------------------------------------ */

void Shelf::initWindows() noexcept
{
    auto &manager { marco->ext.foreignToplevelManager };

    if (!marco->wl.foreignToplevelManager)
        Log(CZWarning, CZLN, "The compositor doesn't support wlr foreign toplevel management: no apps will be shown");

    manager.onAdded.subscribe(this, [this](MForeignToplevel *w){ addWindow(w); });
    manager.onRemoved.subscribe(this, [this](MForeignToplevel *w){ removeWindow(w); });

    for (MForeignToplevel *w : manager.toplevels())
        addWindow(w);

    // Build the model even with no windows yet, so pinned apps show at startup.
    updateModel();
}

void Shelf::addWindow(MForeignToplevel *window) noexcept
{
    if (std::find(m_windows.begin(), m_windows.end(), window) != m_windows.end())
        return;

    m_windows.push_back(window);

    window->onPropsChanged.subscribe(this, [this](CZBitset<MForeignToplevel::Changes> changes){
        if (changes.has(MForeignToplevel::CHAppId | MForeignToplevel::CHState))
            updateModel();
    });

    updateModel();
}

void Shelf::removeWindow(MForeignToplevel *window) noexcept
{
    std::erase(m_windows, window);
    std::erase(m_minimized, window);
    updateModel();
}

void Shelf::updateModel() noexcept
{
    // Minimized windows keep the order they were minimized in: drop the restored ones and append
    // the newly minimized.
    std::erase_if(m_minimized, [](MForeignToplevel *w){ return !w->props().state.has(MForeignToplevel::Minimized); });

    for (MForeignToplevel *w : m_windows)
        if (w->props().state.has(MForeignToplevel::Minimized) &&
            std::find(m_minimized.begin(), m_minimized.end(), w) == m_minimized.end())
            m_minimized.push_back(w);

    // Pinned apps first (even if not running), then the running ones in the order their first
    // window appeared.
    m_apps.clear();

    for (const auto &id : settings.pinnedApps)
        m_apps.push_back({ id, true, {} });

    for (MForeignToplevel *w : m_windows)
    {
        auto app { std::find_if(m_apps.begin(), m_apps.end(), [w](const App &a){ return a.appId == w->props().appId; }) };

        if (app == m_apps.end())
            m_apps.push_back({ w->props().appId, false, { w } });
        else
            app->windows.push_back(w);
    }

    onWindowsChanged.notify();
}

/* --------------------------------- Pinned apps / launching --------------------------------- */

std::string Shelf::appName(const std::string &appId) const noexcept
{
    auto entry { xdgkit ? xdgkit->desktopEntry(appId) : nullptr };
    return entry && !entry->name.empty() ? entry->name : appId;
}

bool Shelf::isPinned(const std::string &appId) const noexcept
{
    return std::find(settings.pinnedApps.begin(), settings.pinnedApps.end(), appId) != settings.pinnedApps.end();
}

void Shelf::setPinned(const std::string &appId, bool pinned) noexcept
{
    if (appId.empty() || isPinned(appId) == pinned)
        return;

    if (pinned)
        settings.pinnedApps.push_back(appId);
    else
        std::erase(settings.pinnedApps, appId);

    Log(CZInfo, CZLN, "{} {}", pinned ? "Pinned" : "Unpinned", appId);
    settings.save();
    updateModel();
}

bool Shelf::launch(const std::string &appId) noexcept
{
    auto entry { xdgkit ? xdgkit->desktopEntry(appId) : nullptr };

    if (!entry || entry->exec.empty())
    {
        Log(CZWarning, CZLN, "Can't launch {}: no desktop entry with an Exec key", appId);
        return false;
    }

    return entry->launch();
}

bool Shelf::Spawn(const std::vector<std::string> &args) noexcept
{
    if (args.empty())
        return false;

    std::vector<char*> argv;
    for (const auto &a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    const pid_t child { fork() };

    if (child < 0)
        return false;

    if (child == 0)
    {
        // Double fork so the process is reparented to init and never becomes our zombie.
        setsid();
        if (fork() != 0) _exit(0);
        syscall(SYS_close_range, 3, ~0U, 0);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    waitpid(child, nullptr, 0);
    return true;
}
