#include <Surfaces/Dock.h>
#include <Nodes/DockFrame.h>
#include <Core/Log.h>
#include <Core/Shelf.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/AK/AKScene.h>
#include <CZ/Core/CZBorderRadius.h>
#include <CZ/Core/Events/CZEvent.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <cmath>

Dock::Dock(MScreen &screen) noexcept :
    MLayerSurface(),
    m_slide(DOCK_SLIDE_MS,
        [this](CZAnimation *a) { setProgress(m_from + (m_to - m_from) * m_ease(a->value())); },
        [this](CZAnimation *) { setProgress(m_to); }),
    m_showTimer([this](CZTimer *) { show(); })
{
    screen.userData = this;
    setScreen(&screen);
    setScope("cz-shelf-dock");
    setLayer(Layer::Top);
    setKeyboardInteractivity(KeyboardInteractivity::None);
    setAnchor(CZEdgeBottom);

    // The central node is the dock rect, sized by its content. Transparent so the compositor's
    // background blur shows through (the fill is DockBackground's), rounded so both the content
    // and the blur mask are.
    setColor(CZAdaptiveColor(0x00000000));
    layout().setWidthAuto();
    layout().setHeightAuto();
    setDecorations(std::make_unique<DockFrame>());

    background.layout().setPositionType(YGPositionTypeAbsolute);
    background.layout().setPosition(YGEdgeLeft, 0.f);
    background.layout().setPosition(YGEdgeTop, 0.f);
    background.layout().setWidthPercent(100.f);
    background.layout().setHeightPercent(100.f);

    // [running apps] | [minimized windows][trash]
    items.layout().setAlignItems(YGAlignCenter);
    apps.layout().setAlignItems(YGAlignCenter);
    others.layout().setAlignItems(YGAlignCenter);

    m_trash = std::make_unique<DockItem>(GetShelf()->assets.trashIcon(), &others);

    GetShelf()->onWindowsChanged.subscribe(this, [this]{ rebuildItems(); });
    rebuildItems();
    applyMetrics();
    updateExclusiveZone();

    // Pointer enter/leave/motion over the whole surface (i.e. its input region).
    scene().installEventFilter(this);

    setProgress(m_autoHide ? 0.0 : 1.0);
    setMapped(true);
}

void Dock::setTileSize(Int32 tile) noexcept
{
    const DockMetrics metrics { DockMetrics::Make(tile) };

    if (metrics.tile == m_metrics.tile)
        return;

    m_metrics = metrics;
    applyMetrics();
    updateExclusiveZone();
    setProgress(m_progress); // The hidden/shown margins depend on the height
}

void Dock::applyMetrics() noexcept
{
    const DockMetrics &m { m_metrics };

    items.layout().setPadding(YGEdgeLeft, m.paddingSide);
    items.layout().setPadding(YGEdgeRight, m.paddingSide);
    items.layout().setPadding(YGEdgeTop, m.paddingTop);
    items.layout().setPadding(YGEdgeBottom, m.paddingBottom);
    items.layout().setGap(YGGutterColumn, m.spacing);
    apps.layout().setGap(YGGutterColumn, m.spacing);
    others.layout().setGap(YGGutterColumn, m.spacing);

    for (auto &item : m_appItems)
        item->setMetrics(m);

    for (auto &item : m_minimizedItems)
        item->setMetrics(m);

    m_trash->setMetrics(m);

    divider.setMetrics(m);
    setBorderRadius(CZBorderRadius::Make(m.radius));

    if (auto *frame { static_cast<DockFrame*>(decorations()) })
        frame->setDockGeometry(m.height, m.radius);

    updateInputRegion();
}

void Dock::rebuildItems() noexcept
{
    auto *shelf { GetShelf() };
    auto &assets { shelf->assets };

    m_appItems.clear();
    m_minimizedItems.clear();

    // Running apps: activate the app's most recent window (restoring it if minimized).
    for (const auto &app : shelf->apps())
    {
        auto item { std::make_unique<DockItem>(assets.appIcon(app.appId), &apps) };
        item->setRunning(true);
        item->onClicked = [appId = app.appId]{
            for (const auto &a : GetShelf()->apps())
            {
                if (a.appId != appId || a.windows.empty())
                    continue;

                // Prefer a window that isn't minimized; else restore the last one.
                MForeignToplevel *target { a.windows.back() };

                for (auto it = a.windows.rbegin(); it != a.windows.rend(); it++)
                    if (!(*it)->props().state.has(MForeignToplevel::Minimized)) { target = *it; break; }

                if (target->props().state.has(MForeignToplevel::Minimized))
                    target->setMinimized(false);

                target->activate();
                return;
            }
        };
        item->setMetrics(m_metrics);
        m_appItems.emplace_back(std::move(item));
    }

    // Minimized windows: a generic window with the app's icon on top. Clicking restores it.
    for (MForeignToplevel *window : shelf->minimized())
    {
        auto item { std::make_unique<DockItem>(assets.windowIcon(), &others) };
        item->setBadge(assets.appIcon(window->props().appId));
        item->onClicked = [window = CZWeak<MForeignToplevel>(window)]{
            if (!window) return;
            window->setMinimized(false);
            window->activate();
        };
        item->setMetrics(m_metrics);
        item->insertBefore(m_trash.get());
        m_minimizedItems.emplace_back(std::move(item));
    }
}

void Dock::suggestedSizeChanged()
{
    // The dock sizes itself from its content; the compositor only echoes that size back.
}

void Dock::layoutEvent(const CZLayoutEvent &event)
{
    MLayerSurface::layoutEvent(event);

    if (event.changes.has(CZLayoutChangeSize))
        updateInputRegion();
}

Int32 Dock::shownMargin() const noexcept
{
    // The surface extends below the dock by the frame's bottom margin: leave the gap on screen.
    return -(DockFrame::BaseMargins().fBottom - DOCK_SCREEN_GAP);
}

Int32 Dock::hiddenMargin() const noexcept
{
    return -(m_metrics.height + DockFrame::BaseMargins().fBottom - DOCK_HIDDEN_VISIBLE);
}

void Dock::updateInputRegion() noexcept
{
    // Surface-local: the dock rect, extended down to the bottom of the surface (below the screen
    // edge while shown). The shadow margins and the headroom above take no input.
    auto *frame { static_cast<DockFrame*>(decorations()) };
    const SkIRect m { frame ? frame->margins() : DockFrame::BaseMargins() };
    const Int32 width { std::max(1, Int32(layout().calculatedWidth())) };
    SkRegion region { SkIRect::MakeXYWH(m.fLeft, m.fTop, width, m_metrics.height + m.fBottom) };
    setCustomInputRegion(&region);
}

void Dock::updateExclusiveZone() noexcept
{
    // Auto-hide reserves nothing. Otherwise reserve the dock and its gap: the zone counts from the
    // anchored edge including the (negative) bottom margin, so add back the part below the screen.
    if (m_autoHide)
        setExclusiveZone(0);
    else
        setExclusiveZone(m_metrics.height + DockFrame::BaseMargins().fBottom);
}

void Dock::setAutoHide(bool enabled) noexcept
{
    if (m_autoHide == enabled)
        return;

    m_autoHide = enabled;
    updateExclusiveZone();

    if (!m_autoHide)
        show();
    else if (!m_pointerInside && !divider.dragging())
        hide();
}

void Dock::show() noexcept
{
    m_showTimer.stop();
    animateTo(1.0);
}

void Dock::hide() noexcept
{
    m_showTimer.stop();
    animateTo(0.0);
}

void Dock::dividerDragEnded() noexcept
{
    // The pointer may have been released outside the dock: hide now that the drag is over.
    if (m_autoHide && !m_pointerInside)
        hide();
}

void Dock::animateTo(Float64 target) noexcept
{
    if (m_slide.isRunning() ? m_to == target : m_progress == target)
        return;

    // Start from wherever the dock is (e.g. reversing mid-slide), taking a proportional time.
    m_slide.pause();
    m_from = m_progress;
    m_to = target;
    m_slide.setDuration(std::max<UInt32>(1, UInt32(std::lround(DOCK_SLIDE_MS * std::fabs(m_to - m_from)))));
    m_slide.reset();
}

void Dock::setProgress(Float64 progress) noexcept
{
    m_progress = std::clamp(progress, 0.0, 1.0);

    const Int32 bottom { Int32(std::lround(hiddenMargin() + (shownMargin() - hiddenMargin()) * m_progress)) };
    setMargin(SkIRect::MakeLTRB(0, 0, 0, bottom));

    // Fully hidden: nothing is drawn (and no blur requested) on the strip left on screen, which is
    // only there to catch the pointer.
    const SkScalar opacity { m_progress > 0.0 ? 1.f : 0.f };
    background.setOpacity(opacity);

    if (auto *frame { static_cast<DockFrame*>(decorations()) })
        frame->setFrameOpacity(opacity);
}

bool Dock::eventFilter(const CZEvent &event, CZObject &target) noexcept
{
    CZ_UNUSED(target)

    switch (event.type())
    {
    case CZEvent::Type::PointerEnter:
        m_pointerInside = true;

        if (m_slide.isRunning() && m_to == 0.0)
            show(); // Re-entered while hiding: come back right away
        else if (m_progress < 1.0 && !m_slide.isRunning())
            m_showTimer.start(DOCK_SHOW_DELAY_MS);
        break;
    case CZEvent::Type::PointerMove:
        // Like macOS, the hidden dock only opens once the pointer rests on the edge.
        if (m_pointerInside && m_progress == 0.0 && !m_slide.isRunning())
            m_showTimer.start(DOCK_SHOW_DELAY_MS);
        break;
    case CZEvent::Type::PointerLeave:
        m_pointerInside = false;
        m_showTimer.stop();

        // Don't hide in the middle of a resize drag (see dividerDragEnded()).
        if (m_autoHide && !divider.dragging())
            hide();
        break;
    default:
        break;
    }

    return false;
}
