#ifndef DOCKITEM_H
#define DOCKITEM_H

#include <Core/Types.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKImageFrame.h>
#include <CZ/AK/Nodes/AKRoundSolidColor.h>
#include <functional>

/**
 * @brief One dock tile: an icon, an optional badge composited on its bottom-right corner (e.g. the
 *        app a minimized window belongs to) and an optional running indicator dot below it.
 */
class DockItem final : public AKContainer
{
public:
    DockItem(std::shared_ptr<RImage> icon, AKNode *parent = nullptr) noexcept;

    void setMetrics(const DockMetrics &metrics) noexcept;

    /// Image shown over the icon's bottom-right corner (nullptr hides it).
    void setBadge(std::shared_ptr<RImage> badge) noexcept;

    /// Shows the running indicator under the icon.
    void setRunning(bool running) noexcept;

    /// Invoked on a left click (press + release over the item).
    std::function<void()> onClicked;

    /// Invoked on a right button press (to open the item's menu).
    std::function<void(const CZPointerButtonEvent &event)> onContextMenu;

    /// Invoked when the pointer enters (true) or leaves (false) the item.
    std::function<void(bool hovered)> onHoverChanged;

    /// Text for the hover label (app name, window title, ...). Evaluated when the label is shown.
    std::function<std::string()> label;

    AKImageFrame icon { this };
    AKImageFrame badge { this };
    AKRoundSolidColor indicator { 0, CZBorderRadius::Make(0), this };

protected:
    void pointerButtonEvent(const CZPointerButtonEvent &event) override;
    void pointerEnterEvent(const CZPointerEnterEvent &event) override;
    void pointerLeaveEvent(const CZPointerLeaveEvent &event) override;

private:
    void updateIndicatorColor() noexcept;
    bool m_pressed { false };
};

#endif // DOCKITEM_H
