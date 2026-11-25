#ifndef SHELF_H
#define SHELF_H

#include <unordered_set>
#include <functional>
#include <Core/Types.h>
#include <Core/Assets.h>
#include <Core/Settings.h>
#include <CZ/Marco/MApp.h>
#include <CZ/XDG/XDGKit.h>
#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZSignal.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

struct sd_bus_slot;
namespace CZ { class CZEventSource; }

/**
 * @brief The Shelf dock app: owns the Marco app, the shared assets, the window model and one Dock
 *        per screen.
 *
 * The window model comes from the wlr foreign toplevel management protocol (Marco's optional
 * MForeignToplevelManagerExt extension). If the compositor doesn't support it, the docks only show
 * the trash.
 *
 * Apps also count as running while their systemd user unit is active (`app-<launcher>-<id>-<random>.scope`
 * and the like, see XDGDesktopEntry::ScopeAppId()), even with no windows: e.g. a music player or a
 * chat app whose window was closed but keeps running in the background, like on macOS. Apps
 * launched from the dock get one (see XDGDesktopEntry::SetScopeLauncher()), Flatpak apps and apps
 * launched by other desktops' launchers too. Tracked through the systemd user manager on the bus.
 */
class Shelf final : public CZObject
{
public:
    Shelf() noexcept;
    ~Shelf() noexcept;

    /// An app shown in the dock: pinned, running, or both.
    struct App
    {
        std::string appId;
        bool pinned { false };
        std::vector<MForeignToplevel*> windows; ///< In the order they appeared
        bool scoped { false }; ///< Its systemd unit is active (running, maybe with no windows)

        /// Whether it's running (has windows, or its processes still run without them).
        bool running() const noexcept { return scoped || !windows.empty(); }
    };

    std::shared_ptr<MApp> marco;
    std::shared_ptr<XDGKit> xdgkit;
    Assets assets;
    Settings settings;

    /// Pinned apps (in pinned order), then the running unpinned ones (in the order they started running).
    const std::vector<App> &apps() const noexcept { return m_apps; }

    bool isPinned(const std::string &appId) const noexcept;

    /**
     * @brief Pins the usual apps found installed (first launch only, see Settings::firstLaunch).
     *
     * For each one (a browser, music, a camera, files, settings, an editor...) the first installed
     * candidate is pinned: known desktop ids of the app itself, its Flatpak and its alternatives.
     */
    void pinDefaultApps() noexcept;

    /**
     * @brief The app id (desktop entry id) of the app a window with app id @p windowAppId belongs to.
     *
     * They usually match, but not always: e.g. Discord's windows are "discord" while its Flatpak
     * entry is com.discordapp.Discord (see XDGKit::desktopEntriesForWindow(): StartupWMClass). If
     * several entries claim it, a pinned one, else one running (an active systemd unit), else the
     * most likely. Without an entry, @p windowAppId itself. Windows are grouped by this id, so they
     * go with the pinned app (and its launch bounce, and its systemd unit).
     */
    std::string appIdForWindow(const std::string &windowAppId) noexcept;

    /// appIdForWindow() of @p window's app id.
    std::string appIdOf(const MForeignToplevel &window) noexcept;

    /// The app's display name (its desktop entry Name), or the app id if it has none.
    std::string appName(const std::string &appId) const noexcept;

    /// Pins / unpins an app (saved to the settings file).
    void setPinned(const std::string &appId, bool pinned) noexcept;

    /// Replaces the pinned apps (and their order), e.g. after reordering them in the dock.
    void setPinnedApps(const std::vector<std::string> &appIds) noexcept;

    /// Dock auto-hide, on every screen (saved in the settings).
    bool autoHide() const noexcept;
    void setAutoHide(bool enabled) noexcept;

    /// Dock icon size, on every screen (saved in the settings).
    Int32 tileSize() const noexcept { return settings.tileSize; }
    void setTileSize(Int32 tile) noexcept;

    /// Launches an app through its desktop entry. Returns false if it couldn't be launched.
    bool launch(const std::string &appId) noexcept;

    /// Runs a command (argv) detached.
    static bool Spawn(const std::vector<std::string> &argv) noexcept;

    /// Opens the home trash (`trash:///`) in the file manager.
    static void OpenTrash() noexcept;

    /// Minimized windows, in the order they were minimized.
    const std::vector<MForeignToplevel*> &minimized() const noexcept { return m_minimized; }

    /// All toplevels, in the order they appeared.
    const std::vector<MForeignToplevel*> &windows() const noexcept { return m_windows; }

    /// Whether Desk (the desktop) is running: the dock then shows a fixed Desk tile first, like the
    /// macOS Dock's Finder. Tracked through its org.cuarzo.Desk session bus name.
    bool deskRunning() const noexcept { return m_deskRunning; }

    /// App id of Desk's windows (its folder windows): they go with the Desk tile, not an app of their own.
    static constexpr const char *DeskAppId { "org.cuarzo.Desk" };

    /// Whether @p window is one of Desk's.
    static bool IsDeskWindow(const MForeignToplevel &window) noexcept;

    /// Desk's windows, in the order they appeared.
    const std::vector<MForeignToplevel*> &deskWindows() const noexcept { return m_deskWindows; }

    /// Asks Desk to open a file manager window (the Desk tile's click with no windows, or its New Window).
    void openDeskWindow() noexcept;

    /**
     * @brief Brings up the most recent of @p windows that isn't minimized, else restores the last one.
     *
     * Clicking an app (or the Desk tile) with windows. Does nothing if @p windows is empty.
     */
    static void ActivateWindows(const std::vector<MForeignToplevel*> &windows) noexcept;

    /// An app is being dragged out of a dock: every screen catches the drop (see DropCatcher).
    void setAppDragging(bool dragging) noexcept;
    bool appDragging() const noexcept { return m_appDragging; }

    /// Whether the home trash has files (the trash tile shows the full icon).
    bool trashFull() const noexcept { return m_trashFull; }

    /// Emitted after trashFull() changes.
    CZSignal<> onTrashChanged;

    /**
     * @brief Moves the files of a dropped `text/uri-list` to the trash.
     *
     * @param fd The read end of the drop's data pipe (owned: closed when done). Read without
     *           blocking the event loop.
     */
    void trashDroppedFiles(Int32 fd) noexcept;

    /**
     * @brief Reads a dropped (or dragged) `text/uri-list` as it arrives, then calls @p done with its
     *        local files.
     *
     * @param fd The read end of the data pipe (owned: closed when done). Read without blocking the
     *           event loop. Nothing is called if it's -1.
     */
    void readDroppedFiles(Int32 fd, std::function<void(std::vector<std::filesystem::path>)> done) noexcept;

    /**
     * @brief Pins the apps of the .desktop files in @p files (other files are ignored), from
     *        position @p index of the pinned apps on.
     *
     * Files outside the applications folders are installed to ~/.local/share/applications first
     * (so they're found by id, like any app). Already pinned apps are moved there.
     *
     * @return Whether any app got pinned (the dock already shows it).
     */
    bool pinDesktopFiles(const std::vector<std::filesystem::path> &files, size_t index) noexcept;

    /// Emitted after apps(), minimized() or deskRunning() change.
    CZSignal<> onWindowsChanged;

private:
    void onScreenPlugged(MScreen &screen) noexcept;
    void onScreenUnplugged(MScreen &screen) noexcept;

    void initWindows() noexcept;
    void initDeskWatch() noexcept;
    void initScopeWatch() noexcept;
    void scheduleScopesRefresh() noexcept;
    void refreshScopes() noexcept;
    void setScopedApps(std::vector<std::string> &&appIds) noexcept;
    void initTrashWatch() noexcept;
    void updateTrashState() noexcept;
    void trashFiles(const std::vector<std::filesystem::path> &paths) noexcept;
    void setDeskRunning(bool running) noexcept;

    /// Crystals' Exposé shown/hidden on the output named @p output (org.cuarzo.Crystals ExposeChanged):
    /// the dock of that screen shows meanwhile.
    void setExposeShown(bool shown, const std::string &output) noexcept;

    /// An app wants attention (Crystals' AppAttention signal): its icon bounces in every dock.
    void requestAttention(const std::string &windowAppId) noexcept;

    /// Whether the screen @p output shows a fullscreen window's workspace (Crystals' FullscreenChanged).
    void setFullscreen(bool fullscreen, const std::string &output) noexcept;
    bool isFullscreen(const std::string &output) const noexcept { return m_fullscreenOutputs.contains(output); }
    void addWindow(MForeignToplevel *window) noexcept;
    void removeWindow(MForeignToplevel *window) noexcept;
    void updateModel() noexcept;

    std::vector<MForeignToplevel*> m_windows; // In the order they appeared
    std::vector<App> m_apps;
    std::vector<std::string> m_scopedApps;   // App ids with an active systemd unit (sorted)
    std::vector<std::string> m_runningOrder; // Running unpinned apps, in the order they started running
    bool m_scopesRefreshQueued { false };
    std::vector<MForeignToplevel*> m_minimized;
    std::vector<MForeignToplevel*> m_deskWindows;
    bool m_deskRunning { false };
    bool m_trashFull { false };
    bool m_appDragging { false };
    Int32 m_inotify { -1 }, m_trashWatch { -1 }, m_trashFilesWatch { -1 };
    std::shared_ptr<CZEventSource> m_inotifySource;
    std::vector<std::shared_ptr<CZEventSource>> m_dropReaders; // Dropped uri-lists being read
    sd_bus_slot *m_deskMatch {};
    sd_bus_slot *m_exposeMatch {};
    sd_bus_slot *m_attentionMatch {};
    sd_bus_slot *m_fullscreenMatch {};
    sd_bus_slot *m_systemdMatch {};
    std::unordered_set<std::string> m_fullscreenOutputs;
    std::shared_ptr<CZBus> m_bus;
};

#endif // SHELF_H
