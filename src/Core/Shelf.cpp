#include <Core/Shelf.h>
#include <Core/Log.h>
#include <Surfaces/Dock.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/XDG/XDGDesktopEntry.h>
#include <CZ/Core/CZBus.h>
#include <CZ/Core/CZEventSource.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/XDG/XDGTrash.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <cerrno>
#include <systemd/sd-bus.h>
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
    initDeskWatch();
    initTrashWatch();

    marco->onScreenPlugged.subscribe(this, [this](MScreen &s){ onScreenPlugged(s); });
    marco->onScreenUnplugged.subscribe(this, [this](MScreen &s){ onScreenUnplugged(s); });

    for (MScreen *s : marco->screens())
        onScreenPlugged(*s);
}

Shelf::~Shelf() noexcept
{
    if (m_deskMatch)
        sd_bus_slot_unref(m_deskMatch);

    m_dropReaders.clear();
    m_inotifySource.reset();

    if (m_inotify >= 0)
        close(m_inotify);

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

    dock->setTileSize(settings.tileSize);
    dock->setAutoHide(autoHide());
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

/* ------------------------------------------ Desk ------------------------------------------ */

static constexpr const char *DeskBusName { "org.cuarzo.Desk" };

void Shelf::initDeskWatch() noexcept
{
    // Kept: the connection closes when its last reference goes away.
    m_bus = CZBus::GetOrMakeUser();
    auto &bus { m_bus };

    if (!bus)
        return;

    // Desk appearing / disappearing...
    sd_bus_match_signal_async(bus->bus(), &m_deskMatch, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameOwnerChanged",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            const char *name {}, *oldOwner {}, *newOwner {};

            if (sd_bus_message_read(m, "sss", &name, &oldOwner, &newOwner) >= 0 && name && strcmp(name, DeskBusName) == 0)
                static_cast<Shelf*>(data)->setDeskRunning(newOwner && newOwner[0]);

            return 0;
        }, nullptr, this);

    // ...and whether it's already running.
    sd_bus_call_method_async(bus->bus(), nullptr, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            int has { 0 };

            if (!sd_bus_message_is_method_error(m, nullptr) && sd_bus_message_read(m, "b", &has) >= 0 && _shelf == data)
                static_cast<Shelf*>(data)->setDeskRunning(has);

            return 0;
        }, this, "s", DeskBusName);

    // Async requests are only queued: send them now (the bus source only waits for input).
    sd_bus_flush(bus->bus());
}

void Shelf::setDeskRunning(bool running) noexcept
{
    if (m_deskRunning == running)
        return;

    m_deskRunning = running;
    Log(CZInfo, CZLN, "Desk {}", running ? "running" : "stopped");
    onWindowsChanged.notify();
}

void Shelf::openDeskWindow() noexcept
{
    if (auto &bus { m_bus })
    {
        sd_bus_call_method_async(bus->bus(), nullptr, DeskBusName, "/org/cuarzo/Desk", "org.cuarzo.Desk", "NewWindow",
                                 nullptr, nullptr, "");
        sd_bus_flush(bus->bus());
    }
}

void Shelf::setAppDragging(bool dragging) noexcept
{
    if (m_appDragging == dragging)
        return;

    m_appDragging = dragging;

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            dock->setCatcherMapped(dragging);
}

/* ------------------------------------------ Trash ------------------------------------------ */

void Shelf::initTrashWatch() noexcept
{
    m_inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    if (m_inotify < 0)
    {
        Log(CZWarning, CZLN, "inotify unavailable: the trash icon won't follow the trash state");
        updateTrashState();
        return;
    }

    m_inotifySource = CZEventSource::Make(m_inotify, EPOLLIN, CZOwn::Borrow, [this](int fd, UInt32, CZEventSource*){
        alignas(inotify_event) char buf[4096];
        while (read(fd, buf, sizeof(buf)) > 0) {}
        updateTrashState();
    });

    updateTrashState();
}

void Shelf::updateTrashState() noexcept
{
    // (Re)watch the trash and its files directory: either may be created, deleted or replaced
    // (e.g. emptied by removing the whole directory).
    if (m_inotify >= 0)
    {
        const std::filesystem::path trash { XDGTrash::HomeDir() };
        std::error_code ec;
        std::filesystem::create_directories(trash / "files", ec);

        // Adding an existing watch just returns it (no removal: that would queue IN_IGNORED events,
        // which would bring us back here endlessly). Deleted directories drop their watch.
        constexpr UInt32 mask { IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF };
        m_trashWatch = inotify_add_watch(m_inotify, trash.c_str(), mask);
        m_trashFilesWatch = inotify_add_watch(m_inotify, (trash / "files").c_str(), mask);
    }

    const bool full { !XDGTrash::IsEmpty() };

    if (full == m_trashFull)
        return;

    m_trashFull = full;
    onTrashChanged.notify();
}

void Shelf::trashFiles(const std::vector<std::filesystem::path> &paths) noexcept
{
    for (const auto &path : paths)
    {
        std::string error;

        switch (XDGTrash::MoveToTrash(path, &error))
        {
        case XDGTrash::Result::Trashed:
            Log(CZInfo, CZLN, "Moved {} to the trash", path.string());
            break;
        case XDGTrash::Result::OtherDevice:
            Spawn({ "gio", "trash", path.string() }); // Its device's own trash
            break;
        case XDGTrash::Result::Failed:
            Log(CZError, CZLN, "Can't move {} to the trash: {}", path.string(), error);
            break;
        }
    }

    updateTrashState();
}

void Shelf::trashDroppedFiles(Int32 fd) noexcept
{
    readDroppedFiles(fd, [this](std::vector<std::filesystem::path> files){ trashFiles(files); });
}

void Shelf::pinDroppedFiles(Int32 fd, size_t index, std::function<void(bool pinned)> done) noexcept
{
    if (fd < 0)
    {
        if (done) done(false);
        return;
    }

    readDroppedFiles(fd, [this, index, done](std::vector<std::filesystem::path> files){
        const bool pinned { pinDesktopFiles(files, index) };
        if (done) done(pinned);
    });
}

void Shelf::readDroppedFiles(Int32 fd, std::function<void(std::vector<std::filesystem::path>)> done) noexcept
{
    if (fd < 0)
        return;

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    auto data { std::make_shared<std::string>() };

    // The source writes the list asynchronously (it may even be busy): read as it arrives.
    auto reader { CZEventSource::Make(fd, EPOLLIN | EPOLLHUP, CZOwn::Own, [this, data, done](int fd, UInt32, CZEventSource *source){
        char buf[4096];
        ssize_t n;

        while ((n = read(fd, buf, sizeof(buf))) > 0)
            data->append(buf, size_t(n));

        if (n < 0 && (errno == EAGAIN || errno == EINTR))
            return; // More to come

        // EOF (or error): done.
        done(XDGTrash::ParseUriList(*data));

        // Drop the source outside its own callback.
        CZTimer::OneShot(0, [this, source](CZTimer*){
            std::erase_if(m_dropReaders, [source](const auto &r){ return r.get() == source; });
        });
    }) };

    if (reader)
        m_dropReaders.emplace_back(std::move(reader));
    else
        close(fd);
}

bool Shelf::pinDesktopFiles(const std::vector<std::filesystem::path> &files, size_t index) noexcept
{
    if (!xdgkit)
        return false;

    std::vector<std::string> pinned { settings.pinnedApps };
    index = std::min(index, pinned.size());
    bool changed { false };

    for (const auto &file : files)
    {
        if (file.extension() != ".desktop")
            continue;

        // Only applications (not links or folders).
        auto dropped { XDGDesktopEntry::Load(file) };

        if (!dropped)
        {
            Log(CZWarning, CZLN, "Not an application, can't be pinned: {}", file.string());
            continue;
        }

        const std::string id { file.stem().string() };

        // Not an installed app (e.g. dropped from the desktop or Downloads): install it, so it's
        // found by its id like any other (launching, icons, matching its windows).
        if (auto installed { xdgkit->desktopEntry(id) }; !installed)
        {
            const char *dataHome { getenv("XDG_DATA_HOME") };
            const char *home { getenv("HOME") };
            const std::filesystem::path dir { dataHome && dataHome[0] == '/' ? std::filesystem::path(dataHome) / "applications" :
                                              std::filesystem::path(home ? home : "") / ".local/share/applications" };
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::filesystem::copy_file(file, dir / file.filename(), std::filesystem::copy_options::skip_existing, ec);

            if (ec)
            {
                Log(CZError, CZLN, "Can't install {}: {}", file.string(), ec.message());
                continue;
            }

            xdgkit->forgetDesktopEntry(id); // Its miss was cached just now

            if (!xdgkit->desktopEntry(id))
            {
                Log(CZError, CZLN, "Installed {} but it isn't found as {}", file.string(), id);
                continue;
            }

            Log(CZInfo, CZLN, "Installed {} to {}", file.string(), dir.string());
        }

        // Pinned there (moved if it already was).
        if (auto it { std::find(pinned.begin(), pinned.end(), id) }; it != pinned.end())
        {
            const size_t old { size_t(it - pinned.begin()) };
            pinned.erase(it);

            if (old < index)
                index--;
        }

        pinned.insert(pinned.begin() + Int64(index++), id);
        changed = true;
        Log(CZInfo, CZLN, "Pinned {} (dropped)", id);
    }

    if (changed)
        setPinnedApps(pinned);

    return changed;
}

/* --------------------------------- Pinned apps / launching --------------------------------- */

std::string Shelf::appName(const std::string &appId) const noexcept
{
    auto entry { xdgkit ? xdgkit->desktopEntry(appId) : nullptr };
    return entry && !entry->name.empty() ? entry->name : appId;
}

bool Shelf::autoHide() const noexcept
{
    // CZ_SHELF_AUTOHIDE=0 keeps the dock shown regardless of the settings.
    if (const char *env { getenv("CZ_SHELF_AUTOHIDE") }; env && strcmp(env, "0") == 0)
        return false;

    return settings.autoHide;
}

void Shelf::setAutoHide(bool enabled) noexcept
{
    if (settings.autoHide != enabled)
    {
        settings.autoHide = enabled;
        settings.save();
    }

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            dock->setAutoHide(autoHide());
}

void Shelf::setTileSize(Int32 tile) noexcept
{
    tile = DockMetrics::Make(tile).tile;

    if (settings.tileSize != tile)
    {
        settings.tileSize = tile;
        settings.save();
    }

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            dock->setTileSize(tile);
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

void Shelf::setPinnedApps(const std::vector<std::string> &appIds) noexcept
{
    if (settings.pinnedApps == appIds)
        return;

    settings.pinnedApps = appIds;
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
