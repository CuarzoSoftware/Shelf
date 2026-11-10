#ifndef DOCKFRAME_H
#define DOCKFRAME_H

#include <Core/Types.h>
#include <CZ/Marco/Nodes/MDecorations.h>
#include <CZ/AK/Nodes/AKNinePatch.h>

/**
 * @brief Dock decorations: the drop shadow and the hairline borders, drawn as a single generated
 *        9-patch around (and over the edges of) the dock rect.
 *
 * The decoration margins also extend the surface:
 * - Below the dock, down past the screen edge (see DOCK_SCREEN_GAP and DOCK_SHOWN_UNDERSCAN); the
 *   shadow there is simply cut, since it's offscreen.
 * - Above it, by the headroom the dock would need to reach DOCK_TILE_SIZE_MAX. This keeps the
 *   surface height constant whatever the dock size, so resizing the dock by dragging its divider
 *   doesn't move the surface's top edge (and the pointer coordinates the drag relies on).
 */
class DockFrame final : public MDecorations
{
public:
    DockFrame() noexcept;

    /// Margins around the dock rect needed by the shadow / screen edge (without the headroom).
    static SkIRect BaseMargins() noexcept;

    void onBind() noexcept override;

    /// Dock rect height and corner radius the frame is generated for. Also updates the headroom.
    void setDockGeometry(Int32 height, Int32 radius) noexcept;

    /// Fades the frame (used while the dock is fully hidden).
    void setFrameOpacity(SkScalar opacity) noexcept { m_patch.setOpacity(opacity); }

private:
    class Patch final : public AKNinePatch
    {
    public:
        Patch(AKNode *parent) noexcept;
        void setDockGeometry(Int32 height, Int32 radius) noexcept;
    protected:
        void onSceneBegin() override;
    private:
        bool m_needsAsset { true };
        Int32 m_height { 0 };
        Int32 m_radius { 0 };
    };

    Patch m_patch { this };
};

#endif // DOCKFRAME_H
