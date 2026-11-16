#include <Nodes/DockFrame.h>
#include <cmath>
#include <Core/Shelf.h>

DockFrame::DockFrame() noexcept : MDecorations()
{
    // The decorations node itself draws nothing (its child does) and takes no input.
    invisibleRegion.setRect(AK_IRECT_INF);
    SkRegion empty;
    setInputRegion(&empty);
}

SkIRect DockFrame::BaseMargins() noexcept
{
    return SkIRect {
        DOCK_SHADOW_EXTENT,                         // L
        DOCK_SHADOW_EXTENT - DOCK_SHADOW_OFFSET_Y,  // T
        DOCK_SHADOW_EXTENT,                         // R
        DOCK_SCREEN_GAP + DOCK_SHOWN_UNDERSCAN      // B (shadow cut here, it's offscreen)
    };
}

void DockFrame::onBind() noexcept
{
    MDecorations::onBind();
}

void DockFrame::setDockGeometry(Int32 height, Int32 radius) noexcept
{
    const SkIRect base { BaseMargins() };
    const Int32 headroom { std::max(0, Int32(std::lround(DockMetrics::Make(DOCK_TILE_SIZE_MAX).height)) - height) };
    setMargins({ base.fLeft, base.fTop + headroom, base.fRight, base.fBottom });

    // The 9-patch covers the dock rect and its base margins, at the bottom of the decorations
    // area (the headroom above it is left empty).
    m_patch.layout().setHeight(base.fTop + height + base.fBottom);
    m_patch.setDockGeometry(height, radius);
}

DockFrame::Patch::Patch(AKNode *parent) noexcept : AKNinePatch({}, {}, parent)
{
    SkRegion empty;
    setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeBottom, 0.f);
    layout().setWidthPercent(100.f);
}

void DockFrame::Patch::setDockGeometry(Int32 height, Int32 radius) noexcept
{
    if (m_height == height && m_radius == radius)
        return;

    m_height = height;
    m_radius = radius;
    m_needsAsset = true;
    addChange(CHImage); // Make sure onSceneBegin() runs for this node
}

void DockFrame::Patch::onSceneBegin()
{
    if (m_height > 0 && (m_needsAsset || changes().testAnyOf(CHLayoutScale, CHColorScheme)))
    {
        m_needsAsset = false;
        const auto &frame { GetShelf()->assets.dockFrame(scale(), colorScheme(), m_height, m_radius, BaseMargins()) };
        setImage(frame.image);
        setCenter(frame.center);
    }

    AKNinePatch::onSceneBegin();
}
