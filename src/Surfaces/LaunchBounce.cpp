#include <Surfaces/LaunchBounce.h>
#include <Surfaces/Dock.h>
#include <Nodes/DockItem.h>
#include <CZ/Core/CZTimer.h>
#include <cmath>

LaunchBounce::LaunchBounce(Dock &dock, std::shared_ptr<RImage> icon) noexcept :
    MSubsurface(&dock),
    m_dock(dock),
    m_anim(DOCK_BOUNCE_MS,
        [this](CZAnimation *a) {
            // Thrown up and falling back (a parabola, like a ball under gravity).
            const SkScalar t { SkScalar(a->value()) };
            const SkScalar u { 2.f * t - 1.f };
            m_height = DOCK_BOUNCE_HEIGHT * m_dock.metrics().tile * (1.f - u * u);
            updatePosition();
        },
        [this](CZAnimation *) {
            m_height = 0.f;
            updatePosition();

            const auto elapsed { std::chrono::steady_clock::now() - m_start };

            if (m_stopping || elapsed >= std::chrono::milliseconds(DOCK_BOUNCE_TIMEOUT_MS))
                finish();
            else
                m_anim.reset(); // next bounce
        })
{
    setColor(CZAdaptiveColor(0x00000000));

    // Purely visual: clicks go to the dock below.
    SkRegion empty;
    setCustomInputRegion(&empty);
    m_icon.setInputRegion(&empty);
    m_icon.setImage(icon);
    m_icon.layout().setWidthPercent(100.f);
    m_icon.layout().setHeightPercent(100.f);

    // The item's icon is hidden only once the copy is about to be on screen (never a frame without
    // either).
    onWillMap.subscribe(this, [this]{
        m_shown = true;

        if (m_item && !m_finished)
            m_item->icon.setIconOpacity(0.f);
    });

    m_start = std::chrono::steady_clock::now();
    m_anim.start();
}

LaunchBounce::~LaunchBounce()
{
    setItem(nullptr);
}

void LaunchBounce::setItem(DockItem *item) noexcept
{
    if (m_item && m_item != item)
        m_item->icon.setIconOpacity(1.f);

    m_item.reset(item);

    if (m_item)
        m_item->icon.setIconOpacity(m_shown && !m_finished ? 0.f : 1.f);

    updatePosition();
}

void LaunchBounce::stop() noexcept
{
    m_stopping = true;
}

void LaunchBounce::updatePosition() noexcept
{
    if (!m_item)
    {
        setMapped(false);
        return;
    }

    // Same size and position as the item's icon (dock surface coordinates), raised by the bounce.
    const SkIRect rect { m_item->icon.worldRect() };

    if (rect.isEmpty()) // Not laid out yet (just created): keep the last position
        return;
    layout().setWidth(rect.width());
    layout().setHeight(rect.height());
    setPos(rect.x(), rect.y() - Int32(std::lround(m_height)));
    setMapped(!m_finished);
}

void LaunchBounce::finish() noexcept
{
    m_finished = true;

    if (m_item)
        m_item->icon.setIconOpacity(1.f);

    setMapped(false);

    CZTimer::OneShot(0, [self = CZWeak<LaunchBounce>(this)](CZTimer *) {
        if (self && self->onFinished)
            self->onFinished();
    });
}
