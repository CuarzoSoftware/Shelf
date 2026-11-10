#ifndef DOCK_H
#define DOCK_H

#include <Core/Types.h>
#include <Core/CubicBezier.h>
#include <Nodes/DockBackground.h>
#include <Nodes/DockDivider.h>
#include <Nodes/DockItem.h>
#include <Surfaces/LaunchBounce.h>
#include <CZ/Marco/Roles/MLayerSurface.h>
#include <CZ/Marco/Roles/MTooltip.h>
#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Marco/Nodes/MMenuAction.h>
#include <CZ/Marco/Nodes/MMenuDivider.h>
#include <functional>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/Core/CZLinearAnimation.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Core/CZWeak.h>
#include <CZ/AK/Nodes/AKText.h>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * @brief The dock shown on one screen: a layer surface anchored to the bottom edge.
 *
 * The surface's central node is the dock rect (transparent, rounded). It's laid out from its
 * content: the running apps, the divider, then the minimized windows and the trash. Its DockBackground child
 * draws the fill/vibrancy and DockFrame (the decorations) the shadow and hairline borders.
 *
 * Auto-hide slides the whole surface down by animating its bottom layer margin. While hidden,
 * DOCK_HIDDEN_VISIBLE units remain on screen (fully transparent) so the pointer reaching the edge
 * still enters it. The input region spans the dock rect down to the bottom of the surface, which
 * reaches DOCK_SHOWN_UNDERSCAN units below the screen edge while shown, so the pointer moving under
 * the floating dock doesn't hide it.
 */
class Dock final : public MLayerSurface
{
public:
    Dock(MScreen &screen) noexcept;

    /// Auto-hide (enabled by default). When disabled the dock stays shown and reserves its area.
    void setAutoHide(bool enabled) noexcept;
    bool autoHide() const noexcept { return m_autoHide; }

    /// Slides the dock in / out (no-op if already there or heading there).
    void show() noexcept;
    void hide() noexcept;

    /**
     * @brief Preferred icon size; everything else scales with it (clamped to DOCK_TILE_SIZE_MIN..MAX).
     *
     * The size actually used (metrics()) is smaller if the dock wouldn't fit the screen width with
     * it (many items, small screen...). It goes back up to the preferred size once it fits again.
     */
    void setTileSize(Int32 tile) noexcept;
    Int32 preferredTileSize() const noexcept { return m_preferredTile; }
    const DockMetrics &metrics() const noexcept { return m_metrics; }

    /// Launches an app (through its desktop entry) and bounces its icon until it shows a window.
    void launchApp(const std::string &appId) noexcept;

    /// Called by the divider when a resize drag ends.
    void dividerDragEnded(bool resized) noexcept;

    /// Builds a context menu: add actions/dividers to the view with the helpers below.
    using MenuBuilder = std::function<void(Dock &dock)>;

    /**
     * @brief Opens a context menu above @p item (a tooltip whose caret points at it).
     *
     * @param event The button press that requested it (used for the popup grab).
     */
    void openMenu(AKNode &item, const CZPointerButtonEvent &event, const MenuBuilder &build) noexcept;

    /// Opens the divider's menu (dock options).
    void openDividerMenu(const CZPointerButtonEvent &event) noexcept;

    /// Closes the context menu, if open.
    void closeMenu() noexcept;

    bool menuOpen() const noexcept { return m_menu && m_menu->mapped(); }

    /// Menu building helpers (only valid inside a MenuBuilder).
    MMenuAction &addMenuAction(const std::string &text, std::function<void()> onClicked, bool enabled = true) noexcept;
    void addMenuDivider() noexcept;

    DockBackground background { this };
    AKContainer items { YGFlexDirectionRow, false, this };
    AKContainer apps { YGFlexDirectionRow, false, &items };
    DockDivider divider { *this, &items };
    AKContainer others { YGFlexDirectionRow, false, &items };

protected:
    bool eventFilter(const CZEvent &event, CZObject &target) noexcept override;
    void layoutEvent(const CZLayoutEvent &event) override;
    void suggestedSizeChanged() override;

private:
    void applyMetrics() noexcept;
    void fitToScreen() noexcept;
    Int32 dockWidth(const DockMetrics &m) const noexcept;
    Int32 availableWidth() const noexcept;
    void rebuildItems() noexcept;
    void menuClosed() noexcept;
    void buildAppMenu(const std::string &appId) noexcept;
    void buildWindowMenu(MForeignToplevel *window) noexcept;
    void buildTrashMenu() noexcept;
    void setupItem(DockItem &item) noexcept;
    void reportThumbnailRects() noexcept;
    SkIRect thumbnailRect(Int32 index, Int32 count) const noexcept;
    void itemHoverChanged(DockItem &item, bool hovered) noexcept;
    void showLabel(DockItem &item) noexcept;
    void hideLabels() noexcept;
    void animateTo(Float64 target) noexcept;
    void setProgress(Float64 progress) noexcept;
    void updateInputRegion() noexcept;
    void updateExclusiveZone() noexcept;
    Int32 shownMargin() const noexcept;
    Int32 hiddenMargin() const noexcept;

    DockMetrics m_metrics { DockMetrics::Make(DOCK_TILE_SIZE_DEFAULT) };
    Int32 m_preferredTile { DOCK_TILE_SIZE_DEFAULT };

    // Running apps, minimized windows (both rebuilt from the Shelf window model) and the trash.
    std::vector<std::unique_ptr<DockItem>> m_appItems;
    std::vector<std::unique_ptr<DockItem>> m_minimizedItems;
    std::unique_ptr<DockItem> m_trash;

    // Launch feedback, by app id.
    std::unordered_map<std::string, std::unique_ptr<LaunchBounce>> m_bounces;

    bool m_autoHide { true };
    bool m_pointerInside { false };

    // 0 = hidden, 1 = shown
    Float64 m_progress { 0.0 };
    Float64 m_from { 0.0 };
    Float64 m_to { 0.0 };

    // Context menu (reused) and its rows (rebuilt each time it opens).
    std::unique_ptr<MTooltip> m_menu;
    std::unique_ptr<MMenuView> m_menuView;
    std::vector<std::unique_ptr<MMenuItem>> m_menuItems;

    /*
     * Hover labels: a text tooltip (with background blur) over the hovered item. Two of them, so
     * switching items maps the new label before unmapping the old one (no gap or flicker).
     */
    struct HoverLabel
    {
        HoverLabel() noexcept;
        MTooltip tooltip;
        AKText text { "", &tooltip };
        CZWeak<DockItem> item;
    };

    std::unique_ptr<HoverLabel> m_labels[2];
    UInt32 m_activeLabel { 0 };
    CZTimer m_labelHideTimer;

    CubicBezier m_ease { DOCK_EASE[0], DOCK_EASE[1], DOCK_EASE[2], DOCK_EASE[3] };
    CZLinearAnimation m_slide;
    CZTimer m_showTimer;
};

#endif // DOCK_H
