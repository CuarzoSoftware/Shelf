#include <Surfaces/Dock.h>
#include <Nodes/DockFrame.h>
#include <Core/Log.h>
#include <Core/Shelf.h>
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
#include <filesystem>

Dock::Dock(MScreen &screen) noexcept :
    MLayerSurface(),
    m_labelHideTimer([this](CZTimer *) { hideLabels(); }),
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

    for (auto &label : m_labels)
    {
        label = std::make_unique<HoverLabel>();

        // Once a label is on screen, drop the other (previous item's) one.
        label->tooltip.onMappedChanged.subscribe(this, [this, l = label.get()]{
            if (!l->tooltip.mapped())
                return;

            for (auto &other : m_labels)
                if (other.get() != l)
                    other->tooltip.setMapped(false);
        });
    }

    m_trash = std::make_unique<DockItem>(GetShelf()->assets.trashIcon(), &others);
    m_trash->label = []{ return std::string("Trash"); };
    setupItem(*m_trash);
    m_trash->onClicked = []{ Shelf::Spawn({ "xdg-open", "trash:///" }); };
    m_trash->onContextMenu = [this](const CZPointerButtonEvent &e){
        openMenu(*m_trash, e, [this](Dock &){ buildTrashMenu(); });
    };

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

    // Apps: activate the app's most recent window (restoring it if minimized), or launch it if it's
    // pinned but not running.
    for (const auto &app : shelf->apps())
    {
        auto item { std::make_unique<DockItem>(assets.appIcon(app.appId), &apps) };
        item->setRunning(!app.windows.empty());
        item->onClicked = [appId = app.appId]{
            for (const auto &a : GetShelf()->apps())
            {
                if (a.appId != appId)
                    continue;

                if (a.windows.empty())
                {
                    GetShelf()->launch(appId);
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
        item->onContextMenu = [this, raw, appId = app.appId](const CZPointerButtonEvent &e){
            openMenu(*raw, e, [this, appId](Dock &){ buildAppMenu(appId); });
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
    hideLabels();
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
        hideLabels();

        // Don't hide in the middle of a resize drag (see dividerDragEnded()) or while a context
        // menu is open (see menuClosed()).
        if (m_autoHide && !divider.dragging() && !menuOpen())
            hide();
        break;
    default:
        break;
    }

    return false;
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

void Dock::openMenu(DockItem &item, const CZPointerButtonEvent &event, const MenuBuilder &build) noexcept
{
    if (!m_menu)
    {
        m_menu = std::make_unique<MTooltip>();
        m_menu->onMappedChanged.subscribe(this, [this]{ if (!m_menu->mapped()) menuClosed(); });
    }

    m_menu->setMapped(false);
    hideLabels(); // No hover labels while a menu is open (see showLabel())

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
        addMenuAction("Open", [appId]{ GetShelf()->launch(appId); });
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
