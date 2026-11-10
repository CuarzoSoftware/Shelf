#ifndef DOCK_H
#define DOCK_H

#include <Core/Types.h>
#include <Core/CubicBezier.h>
#include <Nodes/DockBackground.h>
#include <Nodes/DockDivider.h>
#include <Nodes/DockItem.h>
#include <CZ/Marco/Roles/MLayerSurface.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/Core/CZLinearAnimation.h>
#include <CZ/Core/CZTimer.h>
#include <memory>
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

    /// Icon size; everything else scales with it (clamped to DOCK_TILE_SIZE_MIN..MAX).
    void setTileSize(Int32 tile) noexcept;
    const DockMetrics &metrics() const noexcept { return m_metrics; }

    /// Called by the divider when a resize drag ends.
    void dividerDragEnded() noexcept;

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
    void rebuildItems() noexcept;
    void animateTo(Float64 target) noexcept;
    void setProgress(Float64 progress) noexcept;
    void updateInputRegion() noexcept;
    void updateExclusiveZone() noexcept;
    Int32 shownMargin() const noexcept;
    Int32 hiddenMargin() const noexcept;

    DockMetrics m_metrics { DockMetrics::Make(DOCK_TILE_SIZE_DEFAULT) };

    // Running apps, minimized windows (both rebuilt from the Shelf window model) and the trash.
    std::vector<std::unique_ptr<DockItem>> m_appItems;
    std::vector<std::unique_ptr<DockItem>> m_minimizedItems;
    std::unique_ptr<DockItem> m_trash;

    bool m_autoHide { true };
    bool m_pointerInside { false };

    // 0 = hidden, 1 = shown
    Float64 m_progress { 0.0 };
    Float64 m_from { 0.0 };
    Float64 m_to { 0.0 };

    CubicBezier m_ease { DOCK_EASE[0], DOCK_EASE[1], DOCK_EASE[2], DOCK_EASE[3] };
    CZLinearAnimation m_slide;
    CZTimer m_showTimer;
};

#endif // DOCK_H
