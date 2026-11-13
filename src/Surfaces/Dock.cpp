#include <CZ/Core/CZEase.h>
#include <Surfaces/Dock.h>
#include <Nodes/DockFrame.h>
#include <Core/Log.h>
#include <Core/Shelf.h>
#include <Surfaces/LaunchBounce.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/AK/AKScene.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZBorderRadius.h>
#include <CZ/Core/Events/CZEvent.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <unordered_map>
#include <CZ/AK/AKApp.h>
#include <CZ/XDG/XDGKit.h>
#include <CZ/XDG/XDGDesktopEntry.h>
#include <filesystem>

Dock::Dock(MScreen &screen) noexcept :
    MLayerSurface(),
    m_labelHideTimer([this](CZTimer *) { hideLabels(); }),
    m_slide(DOCK_SLIDE_MS,
        [this](CZAnimation *a) { setProgress(m_from + (m_to - m_from) * m_ease(a->value())); },
        [this](CZAnimation *) { setProgress(m_to); }),
    m_showTimer([this](CZTimer *) { show(); })
{
    // The empty slot for dropped .desktop files (see openPinGap()): never takes input.
    {
        const SkRegion empty;
        m_pinGap.setInputRegion(&empty);
        m_pinGapAnim.setDuration(DOCK_PIN_GAP_MS);
        m_pinGapAnim.onUpdate.subscribe(this, [this](CZAnimation *a){
            setPinGapProgress(m_pinGapFrom + (m_pinGapTo - m_pinGapFrom) * SkScalar(CZEase::OutCubic(a->value())));
        });
        m_pinGapAnim.onFinish.subscribe(this, [this](CZAnimation*){
            setPinGapProgress(m_pinGapTo);

            // Closed: out of the dock.
            if (m_pinGapTo <= 0.f && !m_pinGapHeld)
            {
                m_pinGap.setParent(nullptr);
                m_pinGapOpen = false;
            }
        });
    }

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
    // Center the items when the dock is held at its minimum width (see applyMetrics()).
    layout().setAlignItems(YGAlignCenter);
    layout().setJustifyContent(YGJustifyCenter);
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

    for (auto &label : m_labels)
    {
        label = std::make_unique<HoverLabel>();

        // Once a label is about to be on screen (its first frame is committed), drop the other
        // (previous item's) one: never a frame without either.
        label->tooltip.onWillMap.subscribe(this, [this, l = label.get()]{
            for (auto &other : m_labels)
                if (other.get() != l)
                    other->tooltip.setMapped(false);
        });
    }

    m_trash = std::make_unique<DockItem>(GetShelf()->assets.trashIcon(GetShelf()->trashFull()), &others);
    GetShelf()->onTrashChanged.subscribe(this, [this]{
        m_trash->icon.setImage(GetShelf()->assets.trashIcon(GetShelf()->trashFull()));
    });
    m_trash->label = []{ return std::string("Trash"); };
    setupItem(*m_trash);
    m_trash->onClicked = []{ Shelf::Spawn({ "xdg-open", "trash:///" }); };
    m_trash->onContextMenu = [this](const CZPointerButtonEvent &e){
        openMenu(*m_trash, e, [this](Dock &){ buildTrashMenu(); });
    };


    // Resolution, scale or rotation changes.
    screen.onPropsChanged.subscribe(this, [this](MScreen &, CZBitset<MScreen::Changes>){ fitToScreen(); });



    // Drag & drop over the dock (reordering apps).
    onDNDEnter.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){
        // Files dragged to the screen edge: show the (auto-hidden) dock so they can reach the trash.
        m_dndInside = true;
        if (m_autoHide && !reordering())
        {
            m_showTimer.stop();
            show();
        }
        dragOver(offer, pos);
    });
    onDNDLeave.subscribe(this, [this](MDNDOffer &){ dndLeft(); });
    onDNDMotion.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){ dragOver(offer, pos); });
    onDNDDrop.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){ dropped(offer, pos); });

    GetShelf()->onWindowsChanged.subscribe(this, [this]{ rebuildItems(); });
    rebuildItems();
    applyMetrics();
    updateExclusiveZone();

    m_catcher = std::make_unique<DropCatcher>(*this, screen);

    // Pointer enter/leave/motion over the whole surface (i.e. its input region).
    scene().installEventFilter(this);

    setProgress(m_autoHide ? 0.0 : 1.0);
    setMapped(true);
}

void Dock::setTileSize(Int32 tile) noexcept
{
    m_preferredTile = DockMetrics::Make(tile).tile;
    fitToScreen();
}

Int32 Dock::availableWidth() const noexcept
{
    MScreen *s { screen() };

    if (!s)
        return 0;

    // Logical width of the current mode.
    for (const auto &mode : s->props().modes)
    {
        if (!mode.isCurrent)
            continue;

        const Int32 px { CZ::Is90Transform(s->props().transform) ? mode.size.height() : mode.size.width() };
        return px / std::max(1, s->props().scale) - 2 * DOCK_SCREEN_SIDE_MARGIN;
    }

    return 0; // Unknown yet: no limit
}

Int32 Dock::dockWidth(const DockMetrics &m) const noexcept
{
    // Mirrors the layout: [padding][apps][gap][divider][gap][minimized windows + trash][padding]
    const auto row { [&m](Int32 n) { return n > 0 ? n * m.tile + (n - 1) * m.spacing : 0; } };
    const Int32 apps { row(Int32(m_appItems.size()) + deskSlots()) };
    const Int32 others { row(Int32(m_minimizedItems.size()) + 1) };
    const Int32 width { 2 * m.paddingSide + apps + m.spacing + m.dividerSlot + m.spacing + others };
    return std::max(width, m.minWidth);
}

void Dock::fitToScreen() noexcept
{
    // The largest size up to the preferred one that fits.
    const Int32 available { availableWidth() };
    DockMetrics metrics { DockMetrics::Make(m_preferredTile) };

    if (available > 0)
        while (metrics.tile > DOCK_TILE_SIZE_MIN && dockWidth(metrics) > available)
            metrics = DockMetrics::Make(metrics.tile - 1);

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

    if (m_deskItem)
        m_deskItem->setMetrics(m);

    divider.setMetrics(m);
    setBorderRadius(CZBorderRadius::Make(m.radius));

    // Never narrower than the 9-patch frame can draw without its corners/shadows overlapping
    // (e.g. no apps running and nothing pinned). The items stay centered.
    layout().setMinWidth(m.minWidth);

    if (auto *frame { static_cast<DockFrame*>(decorations()) })
        frame->setDockGeometry(m.height, m.radius);

    updateInputRegion();
}

void Dock::rebuildItems() noexcept
{
    // Not while an app is being dragged (its item must survive): rebuilt when the drag ends.
    if (reordering())
    {
        m_rebuildPending = true;
        return;
    }

    auto *shelf { GetShelf() };
    auto &assets { shelf->assets };

    syncDeskItem();
    m_appItems.clear();

    // Apps: activate the app's most recent window (restoring it if minimized), or launch it if it's
    // pinned but not running.
    for (const auto &app : shelf->apps())
    {
        auto item { std::make_unique<DockItem>(assets.appIcon(app.appId), &apps) };
        item->setRunning(!app.windows.empty());
        item->appId = app.appId;
        item->onClicked = [this, appId = app.appId]{
            for (const auto &a : GetShelf()->apps())
            {
                if (a.appId != appId)
                    continue;

                if (a.windows.empty())
                {
                    launchApp(appId);
                    return;
                }

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
        item->label = [appId = app.appId]{ return GetShelf()->appName(appId); };
        setupItem(*item);
        DockItem *raw { item.get() };
        item->onDragStarted = [this, raw](const CZPointerButtonEvent &press){ startDrag(*raw, press); };
        item->onContextMenu = [this, raw, appId = app.appId](const CZPointerButtonEvent &e){
            openMenu(*raw, e, [this, appId](Dock &){ buildAppMenu(appId); });
        };
        item->setMetrics(m_metrics);

        // An app flying back to its slot (dropped away from the dock) lands on the new item.
        if (m_fly && m_fly->appId() == app.appId)
            m_fly->setItem(item.get());

        // Keep bouncing over the new item; land once the app shows a window.
        if (auto bounce { m_bounces.find(app.appId) }; bounce != m_bounces.end())
        {
            bounce->second->setItem(item.get());

            if (!app.windows.empty())
                bounce->second->stop();
        }

        m_appItems.emplace_back(std::move(item));
    }

    // An open pin slot stays where it was (the new items were appended after it).
    if (m_pinGapOpen)
        placePinGap();

    // Minimized windows: a generic window with the app's icon on top. Clicking restores it. Items
    // are kept across rebuilds so they can grow in / shrink out while the compositor animates the
    // window into / out of them.
    const auto &minimized { shelf->minimized() };

    for (auto &item : m_minimizedItems)
    {
        if (item->removing || (item->window && std::find(minimized.begin(), minimized.end(), item->window.get()) != minimized.end()))
            continue;

        // Restored or closed: shrink out, then drop it.
        item->removing = true;
        item->animateItemScale(0.f, [this, raw = item.get()]{
            CZTimer::OneShot(0, [this, weak = CZWeak<DockItem>(raw)](CZTimer *){
                if (!weak) return;
                std::erase_if(m_minimizedItems, [&weak](const auto &i){ return i.get() == weak.get(); });
                fitToScreen();
                reportThumbnailRects(); // Its window now gets the next free slot
            });
        });
    }

    for (MForeignToplevel *window : minimized)
    {
        const bool exists { std::any_of(m_minimizedItems.begin(), m_minimizedItems.end(),
            [window](const auto &i){ return !i->removing && i->window == window; }) };

        if (exists)
            continue;

        auto item { std::make_unique<DockItem>(assets.windowIcon(), &others) };
        item->window.reset(window);
        item->setBadge(assets.appIcon(window->props().appId));

        // A small version of the window (the compositor's, title bar included) under the app icon.
        // Kept as the generic window icon if the compositor can't capture it.
        window->captureImage([weak = CZWeak<DockItem>(item.get())](std::shared_ptr<RImage> image){
            if (weak && image && !weak->removing)
                weak->icon.setImage(image);
        });
        item->onClicked = [window = CZWeak<MForeignToplevel>(window)]{
            if (!window) return;
            window->setMinimized(false);
            window->activate();
        };
        item->label = [w = CZWeak<MForeignToplevel>(window)]{
            if (!w) return std::string {};
            return w->props().title.empty() ? GetShelf()->appName(w->props().appId) : w->props().title;
        };
        setupItem(*item);
        DockItem *raw { item.get() };
        item->onContextMenu = [this, raw, window](const CZPointerButtonEvent &e){
            openMenu(*raw, e, [this, window](Dock &){ buildWindowMenu(window); });
        };

        item->setMetrics(m_metrics);
        item->setItemScale(0.f);
        item->insertBefore(m_trash.get());
        item->animateItemScale(1.f);
        m_minimizedItems.emplace_back(std::move(item));
    }

    // More/fewer items: shrink to fit the screen, or grow back towards the preferred size.
    fitToScreen();
    reportThumbnailRects();
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

    // Size, tile size or position changes move the thumbnail slots.
    reportThumbnailRects();
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
    else if (!m_pointerInside && !divider.dragging() && !menuOpen())
        hide(); // (if a menu is open, menuClosed() hides it)
}

void Dock::show() noexcept
{
    m_showTimer.stop();
    animateTo(1.0);
}

void Dock::hide() noexcept
{
    m_showTimer.stop();
    hideLabels();
    animateTo(0.0);
}

void Dock::dividerDragEnded(bool resized) noexcept
{
    // Keep the final size (saved, and applied to the docks on the other screens too). Dragging past
    // the screen width is capped, so the size actually reached is what's kept.
    if (resized)
        GetShelf()->setTileSize(m_metrics.tile);

    // The pointer may have been released outside the dock: hide now that the drag is over.
    // An auto-hiding dock stays up until the app lands (the flight is drawn relative to it).
    if (m_autoHide && !m_pointerInside && !m_fly)
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
        hideLabels();

        // Don't hide in the middle of a resize drag (see dividerDragEnded()), while a context
        // menu is open (see menuClosed()) or while an app is being dragged (see endDrag()).
        if (m_autoHide && !divider.dragging() && !menuOpen() && !reordering() && !m_fly)
            hide();
        break;
    default:
        break;
    }

    return false;
}

/* ----------------------------------------- Minimize ----------------------------------------- */

void Dock::reportThumbnailRects() noexcept
{
    /*
     * Every window gets the rect of its minimized thumbnail, so the compositor knows where to animate
     * it to as soon as it's minimized (before the dock even creates the item):
     *  - Minimized windows: their item's final slot.
     *  - Other windows: the slot a new item would take (after the current ones, before the trash).
     *  - Windows being restored keep their item's rect until it's gone (the restore animation starts
     *    there).
     *
     * A window has a single rect: only the dock on the first screen reports it.
     */
    const auto &screens { GetShelf()->marco->screens() };

    if (screens.empty() || screen() != screens.front())
        return;

    Int32 count { 0 };

    for (const auto &item : m_minimizedItems)
        if (!item->removing)
            count++;

    const auto send { [this](MForeignToplevel *window, const SkIRect &rect) {
        if (window && !rect.isEmpty() && (window->rect() != rect || window->surface() != this))
            window->setRect(*this, rect);
    }};

    Int32 index { 0 };

    for (const auto &item : m_minimizedItems)
        if (!item->removing)
            send(item->window, thumbnailRect(index++, count));

    const SkIRect next { thumbnailRect(count, count + 1) };

    for (MForeignToplevel *window : GetShelf()->windows())
    {
        const bool hasItem { std::any_of(m_minimizedItems.begin(), m_minimizedItems.end(),
            [window](const auto &i){ return i->window == window; }) };

        if (!hasItem)
            send(window, next);
    }
}

SkIRect Dock::thumbnailRect(Int32 index, Int32 count) const noexcept
{
    /*
     * Where minimized item `index` (of `count`) will be once fully grown and the dock has
     * re-centered, in this surface's current coordinates. The compositor animates the window
     * towards it while the item may still be growing. Mirrors the layout, like dockWidth().
     */
    const DockMetrics &m { m_metrics };
    const auto row { [&m](Int32 n) { return n > 0 ? n * m.tile + (n - 1) * m.spacing : 0; } };

    const Int32 apps { row(Int32(m_appItems.size()) + deskSlots()) };
    const Int32 itemsWidth { 2 * m.paddingSide + apps + m.spacing + m.dividerSlot + m.spacing + row(count + 1) };
    const Int32 dockWidth { std::max(itemsWidth, m.minWidth) };

    // Left of the item inside the (final) dock rect; the items are centered in it.
    const Int32 x { (dockWidth - itemsWidth) / 2 + m.paddingSide + apps + m.spacing + m.dividerSlot + m.spacing + index * (m.tile + m.spacing) };

    // The surface is centered on the screen: when its width changes it moves by half the change.
    const SkIRect &frame { worldRect() }; // The dock rect, in surface coordinates
    const Int32 shift { (frame.width() - dockWidth) / 2 };

    return SkIRect::MakeXYWH(frame.x() + shift + x, frame.y() + m.paddingTop, m.tile, m.tile);
}

/* ------------------------------------ Reordering (DND) ------------------------------------ */

void Dock::startDrag(DockItem &item, const CZPointerButtonEvent &press) noexcept
{
    if (reordering() || menuOpen())
        return;

    auto it { std::find_if(m_appItems.begin(), m_appItems.end(), [&item](const auto &i){ return i.get() == &item; }) };

    if (it == m_appItems.end())
        return;

    if (!m_dnd)
    {
        m_dnd = std::make_unique<MDND>();
        m_dndIcon = std::make_unique<DockIcon>(nullptr, m_dnd.get());
        m_dndIcon->layout().setPositionType(YGPositionTypeAbsolute);
        m_dnd->onFinished.subscribe(this, [this]{ endDrag(); });

        // Once the drag icon is about to be on screen: hide the item's icon (its slot stays, empty)
        // and the hover label, so there's never a frame without the icon.
        m_dnd->onWillMap.subscribe(this, [this]{
            hideLabels();

            if (m_dragItem && !(m_fly && m_fly->appId() == m_dragAppId))
                m_dragItem->icon.setIconOpacity(0.f);
        });
        m_dnd->onCancelled.subscribe(this, [this]{ endDrag(); });

        // Other clients (e.g. file managers) get the desktop file. The dock itself doesn't need the
        // data (it already knows what is being dragged), so nothing is written for its own type.
        m_dnd->onSend = [this](const std::string &mimeType, Int32 fd){
            if (mimeType != "text/uri-list")
                return;

            auto entry { GetShelf()->xdgkit ? GetShelf()->xdgkit->desktopEntry(m_dragAppId) : nullptr };

            if (!entry)
                return;

            const std::string uri { "file://" + entry->path.string() + "\r\n" };
            [[maybe_unused]] const auto written { write(fd, uri.data(), uri.size()) };
        };
    }

    const std::string &appId { item.appId };

    // The icon under the pointer exactly where it was grabbed, with its shadow: the drag surface
    // extends past the icon by the shadow's margin (DockIcon draws it outside its node).
    m_dndPad = Int32(std::ceil(SkScalar(m_metrics.tile) * Assets::ShadowMargin()));
    m_dndIcon->setImage(GetShelf()->assets.appIcon(appId));
    m_dndIcon->layout().setPosition(YGEdgeLeft, m_dndPad);
    m_dndIcon->layout().setPosition(YGEdgeTop, m_dndPad);
    m_dndIcon->layout().setWidth(m_metrics.tile);
    m_dndIcon->layout().setHeight(m_metrics.tile);
    m_dnd->layout().setWidth(m_metrics.tile + 2 * m_dndPad);
    m_dnd->layout().setHeight(m_metrics.tile + 2 * m_dndPad);
    const SkIRect iconRect { item.icon.worldRect() };
    const SkPoint pointer { AKApp::Get()->pointer().pos() };
    m_dnd->setHotspot(SkIPoint::Make(
        m_dndPad + std::clamp(Int32(pointer.x()) - iconRect.x(), 0, m_metrics.tile),
        m_dndPad + std::clamp(Int32(pointer.y()) - iconRect.y(), 0, m_metrics.tile)));

    std::vector<std::string> mimeTypes { DOCK_APP_MIME_TYPE };

    if (GetShelf()->xdgkit && GetShelf()->xdgkit->desktopEntry(appId))
        mimeTypes.emplace_back("text/uri-list");

    m_dragAppId = appId;

    if (!m_dnd->start(*this, press, mimeTypes, CZBitset<MDNDOffer::Action>(MDNDOffer::Move | MDNDOffer::Copy)))
        return;

    m_dragItem.reset(&item);
    m_dragStartIndex = size_t(it - m_appItems.begin());
    m_droppedOnDock = false;
    GetShelf()->setAppDragging(true);

    // Its slot stays (empty) where it would be dropped: the icon is hidden once the drag icon shows
    // (see the onWillMap handler above).
}

bool Dock::overTrash(MDNDOffer &offer, const SkPoint &pos) const noexcept
{
    // Files (from any client) over the trash tile. Apps being reordered can't be trashed.
    return !reordering() && m_trash && offer.hasMimeType("text/uri-list") &&
           SkRect::Make(m_trash->worldRect()).contains(pos.x(), pos.y());
}

void Dock::setTrashTarget(bool target) noexcept
{
    if (m_trashTarget == target)
        return;

    // Darkened while it would take the drop, like the macOS Dock.
    m_trashTarget = target;
    m_trash->setDropTarget(target);
}

void Dock::dndLeft() noexcept
{
    m_dndInside = false;
    setTrashTarget(false);

    if (!m_pinGapHeld)
        closePinGap(true);

    if (m_autoHide && !m_pointerInside && !reordering() && !menuOpen() && !divider.dragging())
        hide();
}

void Dock::dragOver(MDNDOffer &offer, const SkPoint &pos) noexcept
{
    if (overTrash(offer, pos))
    {
        offer.accept("text/uri-list");
        offer.setActions(MDNDOffer::Move, MDNDOffer::Move);
        setTrashTarget(true);
        return;
    }

    setTrashTarget(false);

    // Files from other apps: .desktop files are pinned where dropped (others are ignored, the list
    // is only read on drop). Copied: the source must never delete them.
    if (canPin(offer))
    {
        offer.accept("text/uri-list");
        offer.setActions(MDNDOffer::Copy, MDNDOffer::Copy);
        openPinGap(size_t(std::clamp(appSlotAt(pos), 0, Int32(pinnedCount()))));
        return;
    }

    closePinGap(true);

    // Otherwise only our own apps can be dropped here.
    if (!reordering() || !offer.hasMimeType(DOCK_APP_MIME_TYPE))
    {
        offer.accept("");
        return;
    }

    offer.accept(DOCK_APP_MIME_TYPE);
    offer.setActions(MDNDOffer::Move, MDNDOffer::Move);
    m_lastDragPos = pos;
    m_lastDragPosValid = true;

    moveDraggedItemTo(size_t(std::clamp(appSlotAt(pos), 0, Int32(m_appItems.size()) - 1)));
}

Int32 Dock::appSlotAt(const SkPoint &pos) const noexcept
{
    // All app slots are the same size.
    const DockMetrics &m { m_metrics };
    const SkScalar start { SkScalar(apps.worldRect().x() + deskSlots() * (m.tile + m.spacing)) }; // After Desk's tile
    return Int32(std::floor((pos.x() - start + m.spacing * 0.5f) / SkScalar(m.tile + m.spacing)));
}

size_t Dock::pinnedCount() const noexcept
{
    size_t n { 0 };

    while (n < m_appItems.size() && GetShelf()->isPinned(m_appItems[n]->appId))
        n++;

    return n;
}

void Dock::setPinGapProgress(SkScalar progress) noexcept
{
    // Grows from nothing to a tile plus its spacing (the negative margin cancels the row's gap, so
    // a closed slot takes no room at all).
    m_pinGapProgress = progress;
    const DockMetrics &m { m_metrics };
    m_pinGap.layout().setWidth(progress * SkScalar(m.tile + m.spacing));
    m_pinGap.layout().setHeight(m.tile);
    m_pinGap.layout().setMargin(YGEdgeRight, -SkScalar(m.spacing));
}

void Dock::placePinGap() noexcept
{
    // Before the app at m_pinGapIndex, or after the last one.
    if (m_pinGapIndex < m_appItems.size())
        m_pinGap.insertBefore(m_appItems[m_pinGapIndex].get());
    else if (!m_appItems.empty())
        m_pinGap.insertAfter(m_appItems.back().get());
}

void Dock::openPinGap(size_t index) noexcept
{
    if (!m_pinGapOpen)
    {
        m_pinGapOpen = true;
        m_pinGapIndex = index;
        m_pinGap.setParent(&apps);
        placePinGap();
        setPinGapProgress(0.f);
    }
    else if (index != m_pinGapIndex)
    {
        // FLIP, like moveDraggedItemTo(): the icons slide from where they were drawn.
        std::unordered_map<DockItem*, SkScalar> before;

        for (auto &item : m_appItems)
            before[item.get()] = item->layout().calculatedLeft();

        m_pinGapIndex = index;
        placePinGap();

        for (auto &i : m_appItems)
            i->slideFrom(0.f);

        scene().root()->layout().calculate();

        for (auto &i : m_appItems)
            i->slideFrom(before[i.get()] - i->layout().calculatedLeft());
    }

    // (Re)open.
    if (m_pinGapTo != 1.f || (!m_pinGapAnim.isRunning() && m_pinGapProgress < 1.f))
    {
        m_pinGapAnim.pause();
        m_pinGapFrom = m_pinGapProgress;
        m_pinGapTo = 1.f;
        m_pinGapAnim.reset();
    }
}

void Dock::closePinGap(bool animated) noexcept
{
    if (!m_pinGapOpen || m_pinGapHeld)
        return;

    if (!animated || m_pinGapProgress <= 0.f)
    {
        m_pinGapAnim.pause();
        m_pinGapTo = 0.f;
        setPinGapProgress(0.f);
        m_pinGap.setParent(nullptr);
        m_pinGapOpen = false;
        return;
    }

    if (m_pinGapTo == 0.f && m_pinGapAnim.isRunning())
        return;

    m_pinGapAnim.pause();
    m_pinGapFrom = m_pinGapProgress;
    m_pinGapTo = 0.f;
    m_pinGapAnim.reset();
}

bool Dock::canPin(MDNDOffer &offer) const noexcept
{
    return !reordering() && offer.hasMimeType("text/uri-list") && !offer.hasMimeType(DOCK_APP_MIME_TYPE);
}

void Dock::dropped(MDNDOffer &offer, const SkPoint &pos) noexcept
{
    if (overTrash(offer, pos))
    {
        // Read the file list asynchronously (the source writes it from its own event loop).
        const Int32 fd { offer.receive("text/uri-list") };
        offer.finish();
        GetShelf()->trashDroppedFiles(fd);
        dndLeft(); // No leave event follows a drop
        return;
    }

    if (canPin(offer))
    {
        // Where the slot was opened. It stays open until the app takes it (or closes if nothing
        // could be pinned).
        const size_t index { m_pinGapOpen ? m_pinGapIndex : size_t(std::clamp(appSlotAt(pos), 0, Int32(pinnedCount()))) };
        const Int32 fd { offer.receive("text/uri-list") };
        offer.finish();
        m_pinGapHeld = m_pinGapOpen;
        dndLeft(); // No leave event follows a drop

        GetShelf()->pinDroppedFiles(fd, index, [this](bool pinned){
            m_pinGapHeld = false;
            closePinGap(!pinned); // Pinned: the new item is already in its place
        });
        return;
    }

    if (!reordering() || !offer.hasMimeType(DOCK_APP_MIME_TYPE))
    {
        dndLeft();
        return;
    }

    dragOver(offer, pos);
    m_droppedOnDock = true;

    // Complete the session (the data itself isn't needed, see onSend).
    if (const Int32 fd { offer.receive(DOCK_APP_MIME_TYPE) }; fd >= 0)
        close(fd);

    offer.finish();
}

void Dock::moveDraggedItemTo(size_t index) noexcept
{
    auto cur { std::find_if(m_appItems.begin(), m_appItems.end(), [this](const auto &i){ return i.get() == m_dragItem.get(); }) };

    if (cur == m_appItems.end() || size_t(cur - m_appItems.begin()) == index)
        return;

    // FLIP: remember where every item is drawn, move the slot, then slide each item from its old
    // place to its new one.
    std::unordered_map<DockItem*, SkScalar> before;

    for (auto &item : m_appItems)
        before[item.get()] = item->layout().calculatedLeft();

    auto dragged { std::move(*cur) };
    m_appItems.erase(cur);
    m_appItems.insert(m_appItems.begin() + std::ptrdiff_t(index), std::move(dragged));

    DockItem *item { m_appItems[index].get() };

    if (index + 1 < m_appItems.size())
        item->insertBefore(m_appItems[index + 1].get());
    else if (index > 0)
        item->insertAfter(m_appItems[index - 1].get());

    for (auto &i : m_appItems)
        i->slideFrom(0.f);

    scene().root()->layout().calculate();

    for (auto &i : m_appItems)
        if (i.get() != m_dragItem.get())
            i->slideFrom(before[i.get()] - i->layout().calculatedLeft());
}

SkPoint Dock::screenOrigin() const noexcept
{
    // Anchored to the bottom edge only: horizontally centered, `margin().bottom` above the edge.
    const SkISize screen { m_catcher ? m_catcher->screenSize() : SkISize::MakeEmpty() };
    const SkISize size { surfaceSize() };
    return SkPoint::Make((screen.width() - size.width()) * 0.5f,
                         SkScalar(screen.height() - margin().fBottom - size.height()));
}

void Dock::setCatcherMapped(bool mapped) noexcept
{
    if (m_catcher)
        m_catcher->setMapped(mapped);
}

void Dock::catcherDragOver(MDNDOffer &offer, const SkPoint &screenPos) noexcept
{
    // Only the dock's own app drags: taken anywhere on the screen.
    if (!GetShelf()->appDragging() || !offer.hasMimeType(DOCK_APP_MIME_TYPE))
    {
        offer.accept("");
        return;
    }

    offer.accept(DOCK_APP_MIME_TYPE);
    offer.setActions(MDNDOffer::Move, MDNDOffer::Move);

    // Over this dock (of the app being dragged): reorder as if the dock got the motion itself.
    if (!reordering())
        return;

    const SkPoint origin { screenOrigin() };
    const SkPoint local { screenPos.x() - origin.x(), screenPos.y() - origin.y() };
    m_lastDragPos = local;
    m_lastDragPosValid = true;

    if (SkRect::Make(worldRect()).contains(local.x(), local.y()))
        dragOver(offer, local);
}

void Dock::startFly(const SkPoint &local) noexcept
{
    // The icon as it was under the pointer (the drag icon's hotspot) flies to the item's slot. The
    // item stays hidden until it lands.
    // (The drag icon's hotspot is relative to its surface, which includes the shadow margin.)
    const SkIPoint hotspot { m_dnd ? m_dnd->hotspot() - SkIPoint::Make(m_dndPad, m_dndPad) : SkIPoint::Make(m_metrics.tile / 2, m_metrics.tile / 2) };
    const SkIRect from { SkIRect::MakeXYWH(Int32(local.x()) - hotspot.x(), Int32(local.y()) - hotspot.y(), m_metrics.tile, m_metrics.tile) };

    m_fly = std::make_unique<DropFly>(*this, GetShelf()->assets.appIcon(m_dragAppId), m_dragAppId, from);
    m_fly->setItem(m_dragItem.get());
    m_fly->onFinished = [this]{
        m_fly.reset();

        // Kept up while it flew (see endDrag()).
        if (m_autoHide && !m_pointerInside && !reordering())
            hide();
    };
}

void Dock::catcherDropped(MDNDOffer &offer, const SkPoint &screenPos) noexcept
{
    if (!GetShelf()->appDragging() || !offer.hasMimeType(DOCK_APP_MIME_TYPE))
        return;

    if (reordering())
    {
        // Over the dock: its final slot. Then the icon flies from where it was released to it.
        const SkPoint origin { screenOrigin() };
        const SkPoint local { screenPos.x() - origin.x(), screenPos.y() - origin.y() };

        if (SkRect::Make(worldRect()).contains(local.x(), local.y()))
            dragOver(offer, local);

        startFly(local);
        m_droppedOnDock = true;
    }
    else
    {
        // Dropped on another screen: the app keeps its new slot (no flight across screens).
        for (MScreen *s : GetShelf()->marco->screens())
            if (auto *dock { static_cast<Dock*>(s->userData) }; dock && dock->reordering())
                dock->m_droppedOnDock = true;
    }

    // Complete the session (the data itself isn't needed, see onSend).
    if (const Int32 fd { offer.receive(DOCK_APP_MIME_TYPE) }; fd >= 0)
        close(fd);

    offer.finish();
}

void Dock::endDrag() noexcept
{
    if (!reordering())
        return;

    DockItem *item { m_dragItem.get() };

    // Wherever it's released (or if the drag gets cancelled), the app keeps the last slot opened
    // for it, like on macOS. Not caught by the drop catcher (e.g. released before it showed up):
    // fly from the last known pointer position.
    if (!m_droppedOnDock && !m_fly)
        startFly(m_lastDragPosValid ? m_lastDragPos : SkPoint::Make(SkRect::Make(item->icon.worldRect()).centerX(), SkRect::Make(item->icon.worldRect()).centerY()));

    m_lastDragPosValid = false;

    // Still hidden while it flies back from where it was dropped (DropFly shows it when it lands).
    if (!m_fly || m_fly->appId() != m_dragAppId)
        item->icon.setIconOpacity(1.f);

    m_dragItem.reset();
    m_droppedOnDock = false;
    GetShelf()->setAppDragging(false);

    // The new order sticks: the pinned apps in dock order, including the dragged one (dropping a
    // running app in place keeps it in the dock, like macOS).
    std::vector<std::string> pinned;

    for (const auto &i : m_appItems)
        if (i->appId == m_dragAppId || GetShelf()->isPinned(i->appId))
            pinned.emplace_back(i->appId);

    GetShelf()->setPinnedApps(pinned);

    if (m_rebuildPending)
    {
        m_rebuildPending = false;
        rebuildItems();
    }

    // An auto-hiding dock stays up until the app lands (the flight is drawn relative to it).
    if (m_autoHide && !m_pointerInside && !m_fly)
        hide();
}

/* -------------------------------------- Launch bounce -------------------------------------- */

void Dock::launchApp(const std::string &appId) noexcept
{
    if (!GetShelf()->launch(appId) || m_bounces.contains(appId))
        return;

    DockItem *item { nullptr };

    for (auto &i : m_appItems)
        if (i->appId == appId) { item = i.get(); break; }

    if (!item)
        return;

    auto bounce { std::make_unique<LaunchBounce>(*this, GetShelf()->assets.appIcon(appId)) };
    bounce->setItem(item);
    bounce->onFinished = [this, appId]{ m_bounces.erase(appId); };
    m_bounces[appId] = std::move(bounce);

    // Its label (shown since the pointer is over it) goes away while it bounces.
    if (const HoverLabel *label { m_labels[m_activeLabel].get() }; label->item == item)
        hideLabels();
}

/* --------------------------------------- Hover labels --------------------------------------- */

Dock::HoverLabel::HoverLabel() noexcept
{
    // Never takes the pointer: it would leave the dock and the item under the label.
    SkRegion empty;
    tooltip.setCustomInputRegion(&empty);
    text.setInputRegion(&empty);
    text.setFontSize(DOCK_LABEL_FONT_SIZE);
    text.setTextColor(AKTheme::Text);
    text.layout().setMargin(YGEdgeHorizontal, 6.f);
    text.layout().setMargin(YGEdgeVertical, 1.f);
}

void Dock::syncDeskItem() noexcept
{
    const bool running { GetShelf()->deskRunning() };

    if (!running)
    {
        if (m_deskItem)
        {
            hideLabels();
            m_deskItem.reset();
        }
        return;
    }

    if (m_deskItem)
        return;

    m_deskItem = std::make_unique<DockItem>(GetShelf()->assets.deskIcon(), &apps);

    // Always first (the app items are appended after it).
    if (!apps.children().empty() && apps.children().front() != m_deskItem.get())
        m_deskItem->insertBefore(apps.children().front());

    m_deskItem->label = []{ return std::string("Desk"); };
    m_deskItem->setRunning(true);
    m_deskItem->onClicked = []{ GetShelf()->openDeskWindow(); };
    m_deskItem->onContextMenu = [this](const CZPointerButtonEvent &e){
        openMenu(*m_deskItem, e, [](Dock &dock){
            dock.addMenuAction("New Window", []{ GetShelf()->openDeskWindow(); });
        });
    };
    // No onDragStarted: it can't be moved.
    setupItem(*m_deskItem);
    m_deskItem->setMetrics(m_metrics);
}

void Dock::setupItem(DockItem &item) noexcept
{
    item.onHoverChanged = [this, &item](bool hovered){ itemHoverChanged(item, hovered); };
}

void Dock::itemHoverChanged(DockItem &item, bool hovered) noexcept
{
    if (hovered)
    {
        m_labelHideTimer.stop();
        showLabel(item);
    }
    else
        m_labelHideTimer.start(DOCK_LABEL_HIDE_DELAY_MS);
}

void Dock::showLabel(DockItem &item) noexcept
{
    // Not while a menu is open (right click), nor while the dock is hidden or sliding.
    if (menuOpen() || m_progress < 1.0 || m_slide.isRunning() || !item.label)
        return;

    // Nor over an app bouncing while it launches.
    if (!item.appId.empty() && m_bounces.contains(item.appId))
        return hideLabels();

    HoverLabel *current { m_labels[m_activeLabel].get() };

    if (current->item == &item && current->tooltip.mapped())
        return;

    const std::string text { item.label() };

    if (text.empty())
        return hideLabels();

    // Another item's label is showing: set up the other one. The current one is unmapped once the
    // new one is mapped (see the constructor).
    if (current->tooltip.mapped())
    {
        m_activeLabel ^= 1;
        current = m_labels[m_activeLabel].get();
    }

    current->item.reset(&item);
    current->text.setText(text);

    // Above the item, the caret pointing at its top edge (like the context menu).
    const SkIRect itemRect { item.worldRect() };
    current->tooltip.pointTo(this, { itemRect.x() + itemRect.width() / 2, Int32(worldRect().y()) - 6 }, CZEdgeBottom);
    current->tooltip.setMapped(true);
}

void Dock::hideLabels() noexcept
{
    m_labelHideTimer.stop();

    for (auto &label : m_labels)
        if (label)
            label->tooltip.setMapped(false);
}

/* -------------------------------------- Context menus -------------------------------------- */

void Dock::openMenu(AKNode &item, const CZPointerButtonEvent &event, const MenuBuilder &build) noexcept
{
    if (!m_menu)
    {
        m_menu = std::make_unique<MTooltip>();
        m_menu->onMappedChanged.subscribe(this, [this]{ if (!m_menu->mapped()) menuClosed(); });

        // The hover label goes away once the menu is about to be on screen (no frame without
        // either). No new labels show while it's open (see showLabel()).
        m_menu->onWillMap.subscribe(this, [this]{ hideLabels(); });
    }

    m_menu->setMapped(false);

    // Rebuild the rows (safe here: no row handler is running).
    m_menuItems.clear();
    m_menuView = std::make_unique<MMenuView>(m_menu.get());
    m_menuView->setCheckGutter(true);
    m_menuView->onItemActivated.subscribe(m_menuView.get(), [this](MMenuItem *){ closeMenu(); });
    build(*this);

    // Above the item, the caret pointing at its top edge (a little above the dock).
    const SkIRect itemRect { item.worldRect() };
    const SkIPoint target { itemRect.x() + itemRect.width() / 2, Int32(worldRect().y()) - 6 };

    m_menu->pointTo(this, target, CZEdgeBottom);
    m_menu->setGrab(&event);
    m_menu->setMapped(true);
}

void Dock::openDividerMenu(const CZPointerButtonEvent &event) noexcept
{
    openMenu(divider, event, [](Dock &dock){
        const bool hiding { GetShelf()->autoHide() };
        dock.addMenuAction(hiding ? "Turn Hiding Off" : "Turn Hiding On", [hiding]{
            GetShelf()->setAutoHide(!hiding);
        });
    });
}

void Dock::closeMenu() noexcept
{
    // Only unmapped: the rows are destroyed the next time a menu opens (this may run from a row's
    // own click handler).
    if (m_menu)
        m_menu->setMapped(false);
}

void Dock::menuClosed() noexcept
{
    if (m_autoHide && !m_pointerInside && !divider.dragging())
        hide();
}

MMenuAction &Dock::addMenuAction(const std::string &text, std::function<void()> onClicked, bool enabled) noexcept
{
    auto action { std::make_unique<MMenuAction>(text, m_menuView.get()) };
    action->onClicked = std::move(onClicked);
    action->setEnabled(enabled);
    MMenuAction &ref { *action };
    m_menuItems.emplace_back(std::move(action));
    return ref;
}

void Dock::addMenuDivider() noexcept
{
    m_menuItems.emplace_back(std::make_unique<MMenuDivider>(m_menuView.get()));
}

void Dock::buildAppMenu(const std::string &appId) noexcept
{
    auto *shelf { GetShelf() };
    const auto &apps { shelf->apps() };
    auto app { std::find_if(apps.begin(), apps.end(), [&appId](const Shelf::App &a){ return a.appId == appId; }) };

    if (app == apps.end())
        return;

    // Windows (macOS lists them at the top, the active one checked).
    for (MForeignToplevel *window : app->windows)
    {
        const auto &props { window->props() };
        auto &action { addMenuAction(props.title.empty() ? appId : props.title,
            [w = CZWeak<MForeignToplevel>(window)]{
                if (!w) return;
                if (w->props().state.has(MForeignToplevel::Minimized))
                    w->setMinimized(false);
                w->activate();
            }) };
        action.setChecked(props.state.has(MForeignToplevel::Activated));
    }

    if (!app->windows.empty())
        addMenuDivider();

    const bool pinned { shelf->isPinned(appId) };
    addMenuAction("Keep in Dock", [appId, pinned]{ GetShelf()->setPinned(appId, !pinned); }).setChecked(pinned);

    addMenuDivider();

    if (app->windows.empty())
    {
        addMenuAction("Open", [this, appId]{ launchApp(appId); });
        return;
    }

    std::vector<CZWeak<MForeignToplevel>> windows;
    for (MForeignToplevel *w : app->windows)
        windows.emplace_back(w);

    addMenuAction("Hide", [windows]{
        for (auto &w : windows)
            if (w) w->setMinimized(true);
    });

    addMenuAction("Quit", [windows]{
        for (auto &w : windows)
            if (w) w->close();
    });
}

void Dock::buildWindowMenu(MForeignToplevel *window) noexcept
{
    CZWeak<MForeignToplevel> w { window };

    addMenuAction("Restore", [w]{
        if (!w) return;
        w->setMinimized(false);
        w->activate();
    });

    addMenuAction("Close", [w]{ if (w) w->close(); });
}

// $XDG_DATA_HOME/Trash (the home trash, see the freedesktop Trash specification).
static std::filesystem::path TrashDir() noexcept
{
    if (const char *data { getenv("XDG_DATA_HOME") }; data && data[0] == '/')
        return std::filesystem::path(data) / "Trash";

    if (const char *home { getenv("HOME") }; home && home[0] == '/')
        return std::filesystem::path(home) / ".local/share/Trash";

    return {};
}

void Dock::buildTrashMenu() noexcept
{
    namespace fs = std::filesystem;
    const fs::path trash { TrashDir() };

    std::error_code ec;
    const bool empty { trash.empty() || !fs::is_directory(trash / "files", ec) || fs::is_empty(trash / "files", ec) };

    addMenuAction("Open", []{ Shelf::Spawn({ "xdg-open", "trash:///" }); });
    addMenuDivider();
    addMenuAction("Empty Trash", [trash]{
        // Remove every trashed item and its .trashinfo (and expunged leftovers).
        for (const char *sub : { "files", "info", "expunged" })
        {
            std::error_code ec;
            const fs::path dir { trash / sub };

            if (!fs::is_directory(dir, ec))
                continue;

            for (const auto &entry : fs::directory_iterator(dir, ec))
            {
                std::error_code rmEc;
                fs::remove_all(entry.path(), rmEc);

                if (rmEc)
                    Log(CZWarning, CZLN, "Failed to remove {} from the trash: {}", entry.path().string(), rmEc.message());
            }
        }

        Log(CZInfo, CZLN, "Emptied the trash");
    }, !empty);
}
