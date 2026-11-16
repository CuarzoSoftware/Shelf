#ifndef DOCK_H
#define DOCK_H

#include <Core/Types.h>
#include <Core/CubicBezier.h>
#include <Nodes/DockBackground.h>
#include <Nodes/DockDivider.h>
#include <Nodes/DockItem.h>
#include <Surfaces/LaunchBounce.h>
#include <Surfaces/DropCatcher.h>
#include <Surfaces/DropFly.h>
#include <CZ/Marco/Roles/MLayerSurface.h>
#include <CZ/Marco/Roles/MTooltip.h>
#include <CZ/Marco/Roles/MDND.h>
#include <Nodes/DockIcon.h>
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

    /// Shown while the compositor's Exposé is (even if it auto-hides), see Shelf::setExposeShown().
    void setExposeShown(bool shown) noexcept;
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

    /// An app is being dragged to a new place.
    bool reordering() const noexcept { return m_dragItem != nullptr; }

    /// Shows / hides the screen-wide drop catcher (see DropCatcher), while any dock's app is dragged.
    void setCatcherMapped(bool mapped) noexcept;

    /// The drag over / dropped on this dock's catcher (@p screenPos in screen coordinates).
    void catcherDragOver(MDNDOffer &offer, const SkPoint &screenPos) noexcept;
    void catcherDropped(MDNDOffer &offer, const SkPoint &screenPos) noexcept;

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
    /// Items in the apps row and in the minimized windows + trash row: as shown now (growing and
    /// shrinking ones by their scale), or once their animations end (@p final).
    void itemCounts(bool final, SkScalar &apps, SkScalar &others) const noexcept;
    static SkScalar itemsWidth(const DockMetrics &m, SkScalar apps, SkScalar others) noexcept;

    /// The tile size the dock fits the screen with (up to the preferred one), see fitToScreen().
    SkScalar fitTile(SkScalar apps, SkScalar others) const noexcept;
    Int32 availableWidth() const noexcept;

    /// Logical width of the screen's current mode (0 if unknown).
    Int32 screenWidth() const noexcept;
    void rebuildItems() noexcept;
    void syncDeskItem() noexcept;
    Int32 deskSlots() const noexcept { return m_deskItem ? 1 : 0; }
    void menuClosed() noexcept;
    void buildAppMenu(const std::string &appId) noexcept;
    void buildWindowMenu(MForeignToplevel *window) noexcept;
    void buildTrashMenu() noexcept;
    void setupItem(DockItem &item) noexcept;
    void reportThumbnailRects() noexcept;
    void startDrag(DockItem &item, const CZPointerButtonEvent &press) noexcept;
    void dragOver(MDNDOffer &offer, const SkPoint &pos) noexcept;
    void dropped(MDNDOffer &offer, const SkPoint &pos) noexcept;
    void moveDraggedItemTo(size_t index) noexcept;
    void endDrag() noexcept;

    /// Top-left corner of this surface on its screen (it's centered at the bottom).
    SkPoint screenOrigin() const noexcept;
    bool overTrash(MDNDOffer &offer, const SkPoint &pos) const noexcept;

    /// Files (e.g. .desktop files from a file manager or the desktop) that may be pinned.
    bool canPin(MDNDOffer &offer) const noexcept;

    /// The app slot under @p pos (0 = the first app after Desk's tile), unclamped.
    Int32 appSlotAt(const SkPoint &pos) const noexcept;

    /// Number of pinned apps (they come first in m_appItems).
    size_t pinnedCount() const noexcept;

    // Pinning dropped .desktop files: an empty slot opens where the app would be pinned, like for
    // a reordered app (growing in, the other icons sliding to make room).
    void openPinGap(size_t index) noexcept;
    void closePinGap(bool animated) noexcept;
    void placePinGap() noexcept;
    void setPinGapProgress(SkScalar progress) noexcept;
    void sizePinGap() noexcept;
    AKContainer m_pinGap { YGFlexDirectionRow, false };
    CZLinearAnimation m_pinGapAnim;
    SkScalar m_pinGapProgress { 0.f }, m_pinGapFrom { 0.f }, m_pinGapTo { 0.f };
    size_t m_pinGapIndex { 0 };
    bool m_pinGapOpen { false };   // In the dock (opening, open or closing)
    bool m_pinGapHeld { false };   // Dropped: kept until the app is pinned (see dropped())
    void setTrashTarget(bool target) noexcept;
    void dndLeft() noexcept;
    SkIRect thumbnailRect(Int32 index, Int32 count) const noexcept;
    void itemHoverChanged(DockItem &item, bool hovered) noexcept;
    void showLabel(DockItem &item) noexcept;

    /// Points @p tooltip at @p item (caret at its top edge), kept DOCK_TOOLTIP_SCREEN_MARGIN away
    /// from the screen's left and right edges.
    void pointTooltip(MTooltip &tooltip, const AKNode &item) noexcept;
    void hideLabels() noexcept;
    void animateTo(Float64 target) noexcept;
    void setProgress(Float64 progress) noexcept;
    void updateInputRegion() noexcept;

    /// Extends each item's input to the gaps around it, the top of the dock and the screen bottom
    /// (see DockItem::hitArea). After items are added, removed or moved, or the metrics change.
    void updateHitAreas() noexcept;
    void updateExclusiveZone() noexcept;
    Int32 shownMargin() const noexcept;
    Int32 hiddenMargin() const noexcept;

    DockMetrics m_metrics { DockMetrics::Make(DOCK_TILE_SIZE_DEFAULT) };
    Int32 m_preferredTile { DOCK_TILE_SIZE_DEFAULT };

    // Running apps, minimized windows (both rebuilt from the Shelf window model) and the trash.
    std::vector<std::unique_ptr<DockItem>> m_appItems;
    std::vector<std::unique_ptr<DockItem>> m_minimizedItems;
    std::unique_ptr<DockItem> m_trash;

    // Desk's tile (while it runs): always the first app, can't be moved (like Finder's).
    std::unique_ptr<DockItem> m_deskItem;

    // Reordering: the drag & drop session (icon surface, reused) and the item being dragged. Its
    // icon is hidden while dragged; its slot moves to wherever it would be dropped.
    std::unique_ptr<MDND> m_dnd;
    std::unique_ptr<DockIcon> m_dndIcon; // With its shadow (like in the dock)
    Int32 m_dndPad { 0 };                // Shadow margin around the icon in the drag surface
    CZWeak<DockItem> m_dragItem;
    std::string m_dragAppId;
    size_t m_dragStartIndex { 0 };
    bool m_droppedOnDock { false };
    bool m_rebuildPending { false };

    // Covers the screen while an app is dragged (the drop is taken anywhere), and the app flying back
    // to its slot after a drop away from the dock.
    std::unique_ptr<DropCatcher> m_catcher;
    std::unique_ptr<DropFly> m_fly;
    void startFly(const SkPoint &local) noexcept;
    SkPoint m_lastDragPos {};     // Latest pointer position seen during the drag (surface coordinates)
    bool m_lastDragPosValid { false };

    // Launch feedback, by app id.
    std::unordered_map<std::string, std::unique_ptr<LaunchBounce>> m_bounces;

    bool m_autoHide { true };
    bool m_dndInside { false };   // A drag & drop session is over the dock (from any client)
    bool m_trashTarget { false }; // Files are being dragged over the trash
    bool m_pointerInside { false };
    bool m_exposeShown { false };

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
