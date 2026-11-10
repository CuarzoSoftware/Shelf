#ifndef SHELF_H
#define SHELF_H

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
        std::vector<MForeignToplevel*> windows; ///< In the order they appeared (empty = not running)
    };

    std::shared_ptr<MApp> marco;
    std::shared_ptr<XDGKit> xdgkit;
    Assets assets;
    Settings settings;

    /// Pinned apps (in pinned order), then the running unpinned ones (in the order they appeared).
    const std::vector<App> &apps() const noexcept { return m_apps; }

    bool isPinned(const std::string &appId) const noexcept;

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

    /// Minimized windows, in the order they were minimized.
    const std::vector<MForeignToplevel*> &minimized() const noexcept { return m_minimized; }

    /// All toplevels, in the order they appeared.
    const std::vector<MForeignToplevel*> &windows() const noexcept { return m_windows; }

    /// Whether Desk (the desktop) is running: the dock then shows a fixed Desk tile first, like the
    /// macOS Dock's Finder. Tracked through its org.cuarzo.Desk session bus name.
    bool deskRunning() const noexcept { return m_deskRunning; }

    /// Asks Desk to open a file manager window (the Desk tile's click).
    void openDeskWindow() noexcept;

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

    /// Emitted after apps(), minimized() or deskRunning() change.
    CZSignal<> onWindowsChanged;

private:
    void onScreenPlugged(MScreen &screen) noexcept;
    void onScreenUnplugged(MScreen &screen) noexcept;

    void initWindows() noexcept;
    void initDeskWatch() noexcept;
    void initTrashWatch() noexcept;
    void updateTrashState() noexcept;
    void trashFiles(const std::vector<std::filesystem::path> &paths) noexcept;
    void setDeskRunning(bool running) noexcept;
    void addWindow(MForeignToplevel *window) noexcept;
    void removeWindow(MForeignToplevel *window) noexcept;
    void updateModel() noexcept;

    std::vector<MForeignToplevel*> m_windows; // In the order they appeared
    std::vector<App> m_apps;
    std::vector<MForeignToplevel*> m_minimized;
    bool m_deskRunning { false };
    bool m_trashFull { false };
    bool m_appDragging { false };
    Int32 m_inotify { -1 }, m_trashWatch { -1 }, m_trashFilesWatch { -1 };
    std::shared_ptr<CZEventSource> m_inotifySource;
    std::vector<std::shared_ptr<CZEventSource>> m_dropReaders; // Dropped uri-lists being read
    sd_bus_slot *m_deskMatch {};
    std::shared_ptr<CZBus> m_bus;
};

#endif // SHELF_H
