#include <Surfaces/DropFly.h>
#include <Surfaces/Dock.h>
#include <Nodes/DockItem.h>
#include <CZ/Core/CZEase.h>
#include <CZ/Core/CZTimer.h>
#include <cmath>

DropFly::DropFly(Dock &dock, std::shared_ptr<RImage> icon, const std::string &appId, const SkIRect &from) noexcept :
    MSubsurface(&dock),
    m_dock(dock),
    m_appId(appId),
    m_from(from),
    m_anim(DOCK_DROP_FLY_MS,
        [this](CZAnimation *a) { update(a->value()); },
        [this](CZAnimation *) {
            update(1.0);
            m_finished = true;

            if (m_item)
                m_item->icon.setIconOpacity(1.f);

            setMapped(false);

            CZTimer::OneShot(0, [self = CZWeak<DropFly>(this)](CZTimer *) {
                if (self && self->onFinished)
                    self->onFinished();
            });
        })
{
    setColor(CZAdaptiveColor(0x00000000));

    // Purely visual.
    SkRegion empty;
    setCustomInputRegion(&empty);
    m_icon.setImage(icon);
    m_icon.layout().setWidthPercent(100.f);
    m_icon.layout().setHeightPercent(100.f);

    update(0.0);
    setMapped(true);

    // Starts once on screen, after the compositor's copy of the drag icon (held at the drop point,
    // same image and place) is gone: no frame without the icon, no visible duplicate.
    onCallbackDone.subscribe(this, [this](UInt32){
        CZTimer::OneShot(DOCK_DROP_FLY_HOLD_MS, [self = CZWeak<DropFly>(this)](CZTimer *){ if (self) self->start(); });
    });
    CZTimer::OneShot(DOCK_DROP_FLY_HOLD_MS + 100, [self = CZWeak<DropFly>(this)](CZTimer *){ if (self) self->start(); });
}

void DropFly::start() noexcept
{
    if (m_started)
        return;

    m_started = true;
    m_anim.start();
}

DropFly::~DropFly()
{
    m_anim.pause();

    if (m_item)
        m_item->icon.setIconOpacity(1.f);
}

void DropFly::setItem(DockItem *item) noexcept
{
    if (m_item && m_item != item)
        m_item->icon.setIconOpacity(1.f);

    m_item.reset(item);

    if (m_item)
        m_item->icon.setIconOpacity(m_finished ? 1.f : 0.f);
}

void DropFly::update(Float64 t) noexcept
{
    // Towards the slot's current place (the dock may still be sliding items around).
    SkIRect to { m_from };

    if (m_item && !m_item->icon.worldRect().isEmpty())
        to = m_item->icon.worldRect();

    const SkScalar e { SkScalar(CZEase::OutCubic(t)) };
    const auto lerp = [e](Int32 a, Int32 b){ return Int32(std::lround(a + (b - a) * e)); };

    layout().setWidth(std::max(1, lerp(m_from.width(), to.width())));
    layout().setHeight(std::max(1, lerp(m_from.height(), to.height())));
    setPos(lerp(m_from.x(), to.x()), lerp(m_from.y(), to.y()));
}
