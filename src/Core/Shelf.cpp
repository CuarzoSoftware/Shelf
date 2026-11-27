#include <Core/Shelf.h>
#include <CZ/XDG/XDGGPU.h>
#include <CZ/Core/Utils/CZProcess.h>
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

    // Apps launched from the dock get their own systemd scope: they count as running until their
    // last process exits, even with no windows (see initScopeWatch())
    XDGDesktopEntry::SetScopeLauncher("cuarzo");
    settings.load();

    if (settings.firstLaunch)
        pinDefaultApps();
    assets.init();
    initWindows();
    initDeskWatch();
    initScopeWatch();
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

    if (m_exposeMatch)
        sd_bus_slot_unref(m_exposeMatch);

    if (m_attentionMatch)
        sd_bus_slot_unref(m_attentionMatch);

    if (m_fullscreenMatch)
        sd_bus_slot_unref(m_fullscreenMatch);

    if (m_systemdMatch)
        sd_bus_slot_unref(m_systemdMatch);

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
    dock->setFullscreen(isFullscreen(screen.props().name));
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

    window->onPropsChanged.subscribe(this, [this, window](CZBitset<MForeignToplevel::Changes> changes){
        // The user went to it: the app's icon stops bouncing for attention
        if (changes.has(MForeignToplevel::CHState) && window->props().state.has(MForeignToplevel::Activated))
            for (MScreen *s : marco->screens())
                if (auto *dock { static_cast<Dock*>(s->userData) })
                    dock->stopAttention(appIdOf(*window));

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

    // Unpinned apps shown: those with windows, and those running without them (an active unit) that
    // are actual apps (a desktop entry shown in menus, not e.g. an autostarted helper)
    std::vector<std::string> running;
    m_deskWindows.clear();

    for (MForeignToplevel *w : m_windows)
    {
        // Desk's go with its tile
        if (IsDeskWindow(*w))
            m_deskWindows.push_back(w);
        else
            running.push_back(appIdOf(*w));
    }

    for (const auto &id : m_scopedApps)
    {
        if (!xdgkit)
            break;

        if (auto entry { xdgkit->desktopEntry(id) }; entry && !entry->noDisplay)
            running.push_back(id);
    }

    // They keep the order they started running in (the first window or the unit, whichever came first)
    const auto contains { [](const auto &list, const std::string &id){ return std::find(list.begin(), list.end(), id) != list.end(); } };

    std::erase_if(m_runningOrder, [&](const std::string &id){ return isPinned(id) || !contains(running, id); });

    for (const auto &id : running)
        if (!isPinned(id) && !contains(m_runningOrder, id))
            m_runningOrder.push_back(id);

    // Pinned apps first (even if not running), then the running ones
    m_apps.clear();

    for (const auto &id : settings.pinnedApps)
        m_apps.push_back({ id, true, {} });

    for (const auto &id : m_runningOrder)
        m_apps.push_back({ id, false, {} });

    for (App &app : m_apps)
        app.scoped = std::binary_search(m_scopedApps.begin(), m_scopedApps.end(), app.appId);

    for (MForeignToplevel *w : m_windows)
    {
        if (IsDeskWindow(*w))
            continue;

        const std::string id { appIdOf(*w) };
        auto app { std::find_if(m_apps.begin(), m_apps.end(), [&id](const App &a){ return a.appId == id; }) };

        if (app != m_apps.end())
            app->windows.push_back(w);
    }

    onWindowsChanged.notify();
}

/* ------------------------------------- systemd scopes ------------------------------------- */

static constexpr const char *SystemdBusName { "org.freedesktop.systemd1" };
static constexpr const char *SystemdPath { "/org/freedesktop/systemd1" };
static constexpr const char *SystemdManager { "org.freedesktop.systemd1.Manager" };

void Shelf::initScopeWatch() noexcept
{
    if (!m_bus)
        return;

    // Units appearing, going away or finishing a job (e.g. started): the list is read again
    sd_bus_match_signal_async(m_bus->bus(), &m_systemdMatch, SystemdBusName, SystemdPath, SystemdManager, nullptr,
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            if (_shelf != data)
                return 0;

            const char *member { sd_bus_message_get_member(m) };
            const char *unit {};

            if (!member)
                return 0;

            if (strcmp(member, "UnitNew") == 0 || strcmp(member, "UnitRemoved") == 0)
                sd_bus_message_read(m, "s", &unit);
            else if (strcmp(member, "JobRemoved") == 0)
            {
                UInt32 job {};
                const char *path {}, *result {};
                sd_bus_message_read(m, "uoss", &job, &path, &unit, &result);
            }
            else if (strcmp(member, "Reloading") == 0)
                unit = "app-";

            if (unit && strncmp(unit, "app-", 4) == 0)
                static_cast<Shelf*>(data)->scheduleScopesRefresh();

            return 0;
        }, nullptr, this);

    // systemd only emits them to subscribed clients
    sd_bus_call_method_async(m_bus->bus(), nullptr, SystemdBusName, SystemdPath, SystemdManager, "Subscribe",
        nullptr, nullptr, "");

    refreshScopes();
    sd_bus_flush(m_bus->bus());
}

void Shelf::scheduleScopesRefresh() noexcept
{
    // Several signals per launch / exit: read the list once they settle
    if (m_scopesRefreshQueued)
        return;

    m_scopesRefreshQueued = true;
    CZTimer::OneShot(100, [this](CZTimer*){
        if (_shelf != this)
            return;

        m_scopesRefreshQueued = false;
        refreshScopes();
    });
}

void Shelf::refreshScopes() noexcept
{
    if (!m_bus)
        return;

    sd_bus_call_method_async(m_bus->bus(), nullptr, SystemdBusName, SystemdPath, SystemdManager, "ListUnitsByPatterns",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            if (_shelf != data)
                return 0;

            if (sd_bus_message_is_method_error(m, nullptr))
            {
                const sd_bus_error *error { sd_bus_message_get_error(m) };
                Log(CZWarning, CZLN, "Can't list the apps' systemd units: {}", error && error->message ? error->message : "unknown error");
                return 0;
            }

            if (sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(ssssssouso)") < 0)
                return 0;

            std::vector<std::string> ids;
            const char *name {}, *description {}, *load {}, *active {}, *sub {}, *following {}, *path {}, *jobType {}, *jobPath {};
            UInt32 jobId {};

            while (sd_bus_message_read(m, "(ssssssouso)", &name, &description, &load, &active, &sub, &following,
                                       &path, &jobId, &jobType, &jobPath) > 0)
                if (auto id { XDGDesktopEntry::ScopeAppId(name ? name : "") }; !id.empty())
                    ids.emplace_back(std::move(id));

            sd_bus_message_exit_container(m);
            static_cast<Shelf*>(data)->setScopedApps(std::move(ids));
            return 0;
        }, this, "asas", 1, "active", 2, "app-*.scope", "app-*.service");

    sd_bus_flush(m_bus->bus());
}

void Shelf::setScopedApps(std::vector<std::string> &&appIds) noexcept
{
    std::sort(appIds.begin(), appIds.end());
    appIds.erase(std::unique(appIds.begin(), appIds.end()), appIds.end());

    if (appIds == m_scopedApps)
        return;

    m_scopedApps = std::move(appIds);
    updateModel();
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

    // Crystals' Exposé: the docks show while it does (from any sender: the compositor)
    sd_bus_match_signal_async(bus->bus(), &m_exposeMatch, nullptr, "/org/cuarzo/Crystals",
        "org.cuarzo.Crystals", "ExposeChanged",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            int shown { 0 };
            const char *output {};

            if (sd_bus_message_read(m, "bs", &shown, &output) >= 0 && _shelf == data)
                static_cast<Shelf*>(data)->setExposeShown(shown, output ? output : "");

            return 0;
        }, nullptr, this);

    // A window of an app wants attention (see Crystals' Attention): its icon bounces
    sd_bus_match_signal_async(bus->bus(), &m_attentionMatch, nullptr, "/org/cuarzo/Crystals",
        "org.cuarzo.Crystals", "AppAttention",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            const char *appId {};

            if (sd_bus_message_read(m, "s", &appId) >= 0 && appId && _shelf == data)
                static_cast<Shelf*>(data)->requestAttention(appId);

            return 0;
        }, nullptr, this);

    // Screens showing a fullscreen window's workspace: their dock auto-hides
    sd_bus_match_signal_async(bus->bus(), &m_fullscreenMatch, nullptr, "/org/cuarzo/Crystals",
        "org.cuarzo.Crystals", "FullscreenChanged",
        [](sd_bus_message *m, void *data, sd_bus_error *) -> int {
            int fullscreen { 0 };
            const char *output {};

            if (sd_bus_message_read(m, "bs", &fullscreen, &output) >= 0 && output && _shelf == data)
                static_cast<Shelf*>(data)->setFullscreen(fullscreen, output);

            return 0;
        }, nullptr, this);

    // Crystals sends the current state when told we're here (e.g. Shelf restarted)
    sd_bus_emit_signal(bus->bus(), "/org/cuarzo/Shelf", "org.cuarzo.Shelf", "Started", "");

    // Async requests are only queued: send them now (the bus source only waits for input).
    sd_bus_flush(bus->bus());
}

void Shelf::setFullscreen(bool fullscreen, const std::string &output) noexcept
{
    if (fullscreen)
        m_fullscreenOutputs.insert(output);
    else
        m_fullscreenOutputs.erase(output);

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) }; dock && s->props().name == output)
            dock->setFullscreen(fullscreen);
}

void Shelf::requestAttention(const std::string &windowAppId) noexcept
{
    // Crystals sends the window's app id
    const std::string appId { appIdForWindow(windowAppId) };

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            dock->requestAttention(appId);
}

void Shelf::setExposeShown(bool shown, const std::string &output) noexcept
{
    // Only that screen's dock (hidden: every dock, in case the screen was renamed meanwhile)
    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            if (!shown || s->props().name == output)
                dock->setExposeShown(shown);
}

void Shelf::setDeskRunning(bool running) noexcept
{
    if (m_deskRunning == running)
        return;

    m_deskRunning = running;
    Log(CZInfo, CZLN, "Desk {}", running ? "running" : "stopped");
    onWindowsChanged.notify();
}

bool Shelf::IsDeskWindow(const MForeignToplevel &window) noexcept
{
    return window.props().appId == DeskAppId;
}

void Shelf::ActivateWindows(const std::vector<MForeignToplevel*> &windows) noexcept
{
    if (windows.empty())
        return;

    // Prefer a window that isn't minimized; else restore the last one.
    MForeignToplevel *target { windows.back() };

    for (auto it = windows.rbegin(); it != windows.rend(); it++)
        if (!(*it)->props().state.has(MForeignToplevel::Minimized)) { target = *it; break; }

    if (target->props().state.has(MForeignToplevel::Minimized))
        target->setMinimized(false);

    target->activate();
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

std::string Shelf::appIdForWindow(const std::string &windowAppId) noexcept
{
    if (!xdgkit)
        return windowAppId;

    const auto &entries { xdgkit->desktopEntriesForWindow(windowAppId) };

    if (entries.empty())
        return windowAppId;

    // Several apps may claim it (e.g. a Flatpak's entry and the user's own one): the one in the
    // dock, else the one running, else the most likely
    for (const auto &entry : entries)
        if (isPinned(entry->id))
            return entry->id;

    for (const auto &entry : entries)
        if (std::binary_search(m_scopedApps.begin(), m_scopedApps.end(), entry->id))
            return entry->id;

    return entries.front()->id;
}

std::string Shelf::appIdOf(const MForeignToplevel &window) noexcept
{
    return appIdForWindow(window.props().appId);
}

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
    tile = DockMetrics::ClampTile(tile);

    if (settings.tileSize != tile)
    {
        settings.tileSize = tile;
        settings.save();
    }

    for (MScreen *s : marco->screens())
        if (auto *dock { static_cast<Dock*>(s->userData) })
            dock->setTileSize(tile);
}

void Shelf::pinDefaultApps() noexcept
{
    if (!xdgkit)
        return;

    // In dock order. Each: candidate desktop ids, the first one installed wins
    static const std::vector<std::vector<const char*>> defaults
    {
        { "org.mozilla.firefox", "firefox", "firefox-esr" },
        { "com.spotify.Client", "spotify", "spotify-client" },
        { "google-chrome", "com.google.Chrome", "google-chrome-stable" },
        { "org.gnome.Snapshot", "org.gnome.Cheese", "cheese", "org.kde.kamoso" },                                     // Camera
        { "org.gnome.Nautilus", "org.kde.dolphin", "nemo", "org.xfce.thunar", "thunar", "pcmanfm-qt", "pcmanfm" },     // Files
        { "org.cuarzo.Settings" },
        { "org.inkscape.Inkscape", "inkscape" },
        { "org.gnome.Calculator", "gnome-calculator", "org.kde.kcalc", "qalculate-gtk", "galculator" },               // Calculator
        { "org.gnome.Calendar", "gnome-calendar", "org.kde.merkuro.calendar", "org.kde.korganizer" },                 // Calendar
        { "org.gnome.TextEditor", "org.gnome.gedit", "gedit", "org.kde.kate", "org.kde.kwrite", "org.xfce.mousepad" }, // Text editor
        { "com.microsoft.VSCode", "code", "com.visualstudio.code", "visual-studio-code", "code-oss" },
        { "antigravity" },
        { "org.qt-project.qtcreator", "io.qt.QtCreator", "qtcreator" },
        { "foot", "org.gnome.Ptyxis", "org.gnome.Console", "org.gnome.Terminal", "org.kde.konsole", "kitty",           // Terminal
          "com.mitchellh.ghostty", "Alacritty", "org.wezfurlong.wezterm", "xfce4-terminal", "qterminal", "xterm" },
        { "com.obsproject.Studio", "obs" },
    };

    settings.pinnedApps.clear();

    for (const auto &candidates : defaults)
    {
        for (const char *id : candidates)
        {
            if (xdgkit->desktopEntry(id))
            {
                settings.pinnedApps.emplace_back(id);
                break;
            }
        }
    }

    Log(CZInfo, CZLN, "First launch: {} default apps pinned", settings.pinnedApps.size());
    settings.save();
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

bool Shelf::launch(const std::string &appId, const XDGGPU *gpu) noexcept
{
    auto entry { xdgkit ? xdgkit->desktopEntry(appId) : nullptr };

    if (!entry || entry->exec.empty())
    {
        Log(CZWarning, CZLN, "Can't launch {}: no desktop entry with an Exec key", appId);
        return false;
    }

    // On a graphics card chosen by the user (see Dock::buildAppMenu()), or as the app prefers
    if (!(gpu ? entry->launch(*gpu) : entry->launch()))
        return false;

    // The user launched it: Crystals shows its first window, wherever it opens (other windows
    // appearing in other workspaces only make their icon bounce, see Crystals' Attention)
    if (m_bus)
    {
        sd_bus_emit_signal(m_bus->bus(), "/org/cuarzo/Shelf", "org.cuarzo.Shelf", "AppLaunched", "s", appId.c_str());

        // Its windows' app id, if it's another one (Crystals matches it against theirs)
        if (!entry->startupWMClass.empty() && entry->startupWMClass != appId)
            sd_bus_emit_signal(m_bus->bus(), "/org/cuarzo/Shelf", "org.cuarzo.Shelf", "AppLaunched", "s", entry->startupWMClass.c_str());

        sd_bus_flush(m_bus->bus());
    }

    return true;
}

bool Shelf::Spawn(const std::vector<std::string> &args) noexcept
{
    // Detached, started from a background thread: never blocks the UI
    return CZProcess::SpawnDetached(args);
}

void Shelf::OpenTrash() noexcept
{
    // Not xdg-open: there's usually no x-scheme-handler/trash, so it falls back to the web browser.
    // GIO resolves trash:/// through GVfs (inode/directory), which opens the default file manager.
    Spawn({ "gio", "open", "trash:///" });
}
