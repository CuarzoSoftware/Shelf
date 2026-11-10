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

    /// Emitted after apps() or minimized() change.
    CZSignal<> onWindowsChanged;

private:
    void onScreenPlugged(MScreen &screen) noexcept;
    void onScreenUnplugged(MScreen &screen) noexcept;

    void initWindows() noexcept;
    void addWindow(MForeignToplevel *window) noexcept;
    void removeWindow(MForeignToplevel *window) noexcept;
    void updateModel() noexcept;

    std::vector<MForeignToplevel*> m_windows; // In the order they appeared
    std::vector<App> m_apps;
    std::vector<MForeignToplevel*> m_minimized;
};

#endif // SHELF_H
