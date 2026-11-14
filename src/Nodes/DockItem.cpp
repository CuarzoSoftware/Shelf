#include <Core/Shelf.h>
#include <Nodes/DockItem.h>
#include <CZ/AK/AKApp.h>
#include <CZ/AK/AKScene.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Core/CZEase.h>
#include <CZ/Core/Events/CZPointerMoveEvent.h>
#include <cmath>
#include <CZ/Core/Events/CZPointerEnterEvent.h>
#include <CZ/Core/Events/CZPointerLeaveEvent.h>

DockItem::DockItem(std::shared_ptr<RImage> image, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionColumn, false, parent),
    m_scaleAnim(DOCK_MINIMIZE_MS,
        [this](CZAnimation *a) {
            setItemScale(m_scaleFrom + (m_scaleTo - m_scaleFrom) * SkScalar(CZEase::InOutCubic(a->value())));
        },
        [this](CZAnimation *) {
            setItemScale(m_scaleTo);
            if (auto done { std::move(m_scaleDone) })
                done();
        }),
    m_slideAnim(DOCK_REORDER_MS,
        [this](CZAnimation *a) { setSlideOffset(m_slideFrom * (1.f - SkScalar(CZEase::OutCubic(a->value())))); },
        [this](CZAnimation *) { setSlideOffset(0.f); })
{
    SkRegion empty;

    icon.setImage(image);
    icon.setInputRegion(&empty);
    icon.layout().setWidthPercent(100.f);
    icon.layout().setHeightPercent(100.f);

    badge.setInputRegion(&empty);
    badge.layout().setPositionType(YGPositionTypeAbsolute);
    badge.layout().setPosition(YGEdgeRight, 0.f);
    badge.layout().setPosition(YGEdgeBottom, 0.f);
    badge.setVisible(false);

    // The item's input is its hit area's (see hitArea)
    setInputRegion(&empty);
    hitArea.layout().setPositionType(YGPositionTypeAbsolute);
    setHitExtents(0, 0, 0, 0);

    indicator.setInputRegion(&empty);
    indicator.layout().setPositionType(YGPositionTypeAbsolute);
    indicator.setVisible(false);
    indicator.onColorSchemeChanged.subscribe(this, [this](CZColorScheme){ updateIndicatorColor(); });
    updateIndicatorColor();
}

void DockItem::setHitExtents(Int32 left, Int32 top, Int32 right, Int32 bottom) noexcept
{
    // Both edges of each axis: it follows the item's size (minimize animations, ...)
    hitArea.layout().setPosition(YGEdgeLeft, -left);
    hitArea.layout().setPosition(YGEdgeTop, -top);
    hitArea.layout().setPosition(YGEdgeRight, -right);
    hitArea.layout().setPosition(YGEdgeBottom, -bottom);
}

void DockItem::setMetrics(const DockMetrics &m) noexcept
{
    m_metrics = m;
    applySize();
}

void DockItem::setItemScale(SkScalar scale) noexcept
{
    scale = std::clamp(scale, 0.f, 1.f);

    if (scale == m_scale)
        return;

    m_scale = scale;
    applySize();
}

void DockItem::animateItemScale(SkScalar target, std::function<void()> done) noexcept
{
    m_scaleAnim.pause();
    m_scaleFrom = m_scale;
    m_scaleTo = target;
    m_scaleDone = std::move(done);
    m_scaleAnim.setDuration(std::max<UInt32>(1, UInt32(DOCK_MINIMIZE_MS * std::fabs(m_scaleTo - m_scaleFrom))));
    m_scaleAnim.reset();
}

void DockItem::applySize() noexcept
{
    const DockMetrics &m { m_metrics };
    const SkScalar tile { m.tile * m_scale };
    layout().setWidth(tile);
    layout().setHeight(tile);

    // The badge covers ~45% of the tile, like the app icon on a minimized window in the macOS Dock.
    const SkScalar badgeSize { tile * 0.45f };
    badge.layout().setWidth(badgeSize);
    badge.layout().setHeight(badgeSize);

    // A small dot centered in the bottom padding.
    const Int32 dot { std::max(3, (m.tile * 4 + DOCK_TILE_SIZE_DEFAULT / 2) / DOCK_TILE_SIZE_DEFAULT) };
    indicator.layout().setWidth(dot);
    indicator.layout().setHeight(dot);
    indicator.layout().setPosition(YGEdgeLeft, (m.tile - dot) / 2);
    indicator.layout().setPosition(YGEdgeTop, m.tile + std::max(0, (m.paddingBottom - dot) / 2));
    indicator.setBorderRadius(CZBorderRadius::Make(dot / 2));
}

void DockItem::setBadge(std::shared_ptr<RImage> image) noexcept
{
    // Sized for each screen's scale (see Assets::provider()).
    if (image)
        badge.setImageProvider(GetShelf()->assets.provider(image));
    else
        badge.setImage(nullptr);

    badge.setVisible(image != nullptr);
}

void DockItem::setDropTarget(bool target) noexcept
{
    m_dropTarget = target;
    updateDarkening();
}

void DockItem::setPressed(bool pressed) noexcept
{
    m_darkPressed = pressed;
    updateDarkening();
}

void DockItem::updateDarkening() noexcept
{
    // Multiplies the colors (rather than fading): darker over any background.
    const SkScalar f { (m_dropTarget || m_darkPressed) ? DOCK_ITEM_DARKEN : 1.f };
    icon.setColorFactor({ f, f, f, 1.f });
    badge.setColorFactor({ f, f, f, 1.f });
}

void DockItem::setRunning(bool running) noexcept
{
    indicator.setVisible(running);
}

void DockItem::updateIndicatorColor() noexcept
{
    indicator.setBackgroundColor(DOCK_INDICATOR_COLOR.resolve(indicator.colorScheme()));
}

void DockItem::pointerButtonEvent(const CZPointerButtonEvent &event)
{
    AKContainer::pointerButtonEvent(event);

    if (removing)
        return;

    if (event.button == BTN_RIGHT && event.pressed)
    {
        m_pressed = false;
        setPressed(false);
        if (onContextMenu)
            onContextMenu(event);
        return;
    }

    if (event.button != BTN_LEFT)
        return;

    if (event.pressed)
    {
        m_pressed = true;
        setPressed(true);
        m_press = std::static_pointer_cast<CZPointerButtonEvent>(event.copy());
        m_pressPos = AKApp::Get()->pointer().pos();
    }
    else if (m_pressed)
    {
        m_pressed = false;
        setPressed(false);
        m_press.reset();

        // Released over the item (not dragged away).
        if (scene() && scene()->pointerFocus() && (scene()->pointerFocus() == this || scene()->pointerFocus()->isSubchildOf(this)) && onClicked)
            onClicked();
    }
}

void DockItem::pointerEnterEvent(const CZPointerEnterEvent &event)
{
    AKContainer::pointerEnterEvent(event);

    if (onHoverChanged)
        onHoverChanged(true);
}

void DockItem::pointerLeaveEvent(const CZPointerLeaveEvent &event)
{
    AKContainer::pointerLeaveEvent(event);

    setPressed(false);

    // Left with the button held: dragged out quickly.
    if (m_pressed && m_press && onDragStarted && !removing)
    {
        m_pressed = false;
        auto press { std::move(m_press) };
        onDragStarted(*press);
    }

    m_pressed = false;
    m_press.reset();

    if (onHoverChanged)
        onHoverChanged(false);
}

void DockItem::pointerMoveEvent(const CZPointerMoveEvent &event)
{
    AKContainer::pointerMoveEvent(event);

    if (!m_pressed || !m_press || removing || !onDragStarted)
        return;

    const SkPoint pos { AKApp::Get()->pointer().pos() };

    if (SkPoint::Distance(pos, m_pressPos) < DOCK_DRAG_THRESHOLD)
        return;

    // A drag, not a click.
    m_pressed = false;
    setPressed(false);
    auto press { std::move(m_press) };
    onDragStarted(*press);
}

void DockItem::slideFrom(SkScalar offset) noexcept
{
    if (std::fabs(offset) < 0.5f)
    {
        m_slideAnim.pause();
        setSlideOffset(0.f);
        return;
    }

    m_slideFrom = offset;
    setSlideOffset(offset);
    m_slideAnim.reset();
}

void DockItem::setSlideOffset(SkScalar offset) noexcept
{
    m_slideOffset = offset;

    // A relative offset: moves the item without affecting the layout of the others.
    layout().setPosition(YGEdgeLeft, offset);
}
