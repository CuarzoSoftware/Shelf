#ifndef SHELF_H
#define SHELF_H

#include <Core/Types.h>
#include <Core/Assets.h>
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

    /// The windows of one running app.
    struct App
    {
        std::string appId;
        std::vector<MForeignToplevel*> windows; ///< In the order they appeared
    };

    std::shared_ptr<MApp> marco;
    std::shared_ptr<XDGKit> xdgkit;
    Assets assets;

    /// Running apps, grouped by app id, in the order they first appeared.
    const std::vector<App> &apps() const noexcept { return m_apps; }

    /// Minimized windows, in the order they were minimized.
    const std::vector<MForeignToplevel*> &minimized() const noexcept { return m_minimized; }

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
