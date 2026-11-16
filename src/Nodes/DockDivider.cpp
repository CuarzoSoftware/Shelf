#include <Nodes/DockDivider.h>
#include <Surfaces/Dock.h>
#include <Core/Shelf.h>
#include <CZ/AK/AKApp.h>
#include <CZ/AK/AKScene.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Core/Events/CZPointerMoveEvent.h>
#include <cmath>

DockDivider::DockDivider(Dock &dock, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionColumn, false, parent),
    m_dock(dock)
{
    layout().setJustifyContent(YGJustifyCenter);
    layout().setAlignItems(YGAlignCenter);
    layout().setAlignSelf(YGAlignStretch);
    setCursor(CZCursorShape::ResizeTB);
}

void DockDivider::setMetrics(const DockMetrics &metrics) noexcept
{
    layout().setWidth(metrics.dividerSlot);
    m_line.layout().setHeight(metrics.dividerHeight);
}

Float32 DockDivider::pointerFromBottom() const noexcept
{
    return Float32(m_dock.surfaceSize().height()) - AKApp::Get()->pointer().pos().y();
}

void DockDivider::pointerButtonEvent(const CZPointerButtonEvent &event)
{
    AKContainer::pointerButtonEvent(event);

    if (event.button == BTN_RIGHT && event.pressed && !m_dragging)
    {
        m_dock.openDividerMenu(event);
        return;
    }

    if (event.button != BTN_LEFT)
        return;

    if (event.pressed && !m_dragging)
    {
        m_dragging = true;
        m_dragStartFromBottom = pointerFromBottom();
        m_dragStartTile = Int32(std::lround(m_dock.metrics().tile));
        m_resized = false;

        // Keep getting the motion while the pointer leaves the divider (or the dock).
        if (scene())
            scene()->setPointerGrab(this);
    }
    else if (!event.pressed && m_dragging)
        endDrag();
}

void DockDivider::pointerMoveEvent(const CZPointerMoveEvent &event)
{
    AKContainer::pointerMoveEvent(event);

    if (!m_dragging)
        return;

    // The dock grows with the pointer: one unit of tile size per unit dragged up.
    const Int32 delta { Int32(std::lround(pointerFromBottom() - m_dragStartFromBottom)) };
    // Not until it actually moves: a plain click must not turn a size shrunk to fit the screen
    // into the preferred one.
    if (delta == 0 && !m_resized)
        return;

    m_resized = true;
    m_dock.setTileSize(m_dragStartTile + delta);
}

void DockDivider::endDrag() noexcept
{
    m_dragging = false;

    if (scene() && scene()->pointerGrab() == this)
        scene()->setPointerGrab(nullptr);

    m_dock.dividerDragEnded(m_resized);
}

DockDivider::Line::Line(AKNode *parent) noexcept : AKThreePatch(CZOrientation::V, parent)
{
    SkRegion empty;
    setInputRegion(&empty);
    layout().setWidth(1);

    // A 1x3 white image: top cap, stretched middle row, bottom cap. Tinted by the node color.
    setImage(GetShelf()->assets.dividerImage());
    setImageScale(1);
    setSideSrcRect(SkRect::MakeXYWH(0, 0, 1, 1));
    setCenterSrcRect(SkRect::MakeXYWH(0, 1, 1, 1));
    enableReplaceImageColor(true);
    setColor(DOCK_DIVIDER_COLOR.resolve(colorScheme()));
}

void DockDivider::Line::onSceneBegin()
{
    if (changes().test(CHColorScheme))
        setColor(DOCK_DIVIDER_COLOR.resolve(colorScheme()));

    AKThreePatch::onSceneBegin();
}
