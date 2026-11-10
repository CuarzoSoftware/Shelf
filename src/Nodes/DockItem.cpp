#include <Nodes/DockItem.h>
#include <CZ/AK/AKApp.h>
#include <CZ/AK/AKScene.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Core/CZEase.h>
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
        })
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

    indicator.setInputRegion(&empty);
    indicator.layout().setPositionType(YGPositionTypeAbsolute);
    indicator.setVisible(false);
    indicator.onColorSchemeChanged.subscribe(this, [this](CZColorScheme){ updateIndicatorColor(); });
    updateIndicatorColor();
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
    badge.setImage(image);
    badge.setVisible(image != nullptr);
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
        if (onContextMenu)
            onContextMenu(event);
        return;
    }

    if (event.button != BTN_LEFT)
        return;

    if (event.pressed)
        m_pressed = true;
    else if (m_pressed)
    {
        m_pressed = false;

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
    m_pressed = false;

    if (onHoverChanged)
        onHoverChanged(false);
}
