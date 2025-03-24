#include <AK/events/AKLayoutEvent.h>
#include <AK/AKLog.h>
#include <Marco/MScreen.h>
#include <Theme.h>
#include <Dock.h>

Dock::Dock(MScreen *screen) noexcept :
    MLayerSurface(MLayerSurface::Overlay, AKEdgeBottom, -1, screen, "cuarzo-shelf")
{
    AKLog::debug("New Dock for Screen %s.", screen->props().name.c_str());
    layout().setHeight(Theme::dockTextureHeight(Theme::MAX_DOCK_HEIGHT));
    layout().setWidth(screenWidth());
    setColorWithAlpha(SK_ColorTRANSPARENT);
    setupAnimations();
    setupBackground();
    setMargin(SkIRect::MakeLTRB(0, 0, 0, -(layout().height().value - 2)));
}

SkScalar Dock::screenWidth() const noexcept
{
    if (screen() && !screen()->props().modes.empty())
        return screen()->props().modes[0].size.width() / screen()->props().scale;

    return 1024.f;
}

void Dock::pointerEnterEvent(const AKPointerEnterEvent &e)
{
    MLayerSurface::pointerEnterEvent(e);
    hover = true;
    showDock();
    AKLog::debug("Pointer enter.");
}

void Dock::pointerLeaveEvent(const AKPointerLeaveEvent &e)
{
    MLayerSurface::pointerLeaveEvent(e);
    hover = false;
    hideDock();
    AKLog::debug("Pointer leave.");
}

void Dock::showDock() noexcept
{
    if (slideUpAnim.running() || slideDownAnim.running())
        return;

    slideUpAnim.start();
}

void Dock::hideDock() noexcept
{
    if (slideUpAnim.running() || slideDownAnim.running())
        return;

    slideDownAnim.start();
}

void Dock::slideUpAnimUpdate() noexcept
{
    setMargin(SkIRect::MakeLTRB(0, 0, 0, -2 -(layout().height().value - 2) * (1.f - slideUpAnim.value())));
    update();
}

void Dock::slideUpAnimFinish() noexcept
{
    setMargin(SkIRect::MakeLTRB(0, 0, 0, -1));

    if (!hover)
        hideDock();
}

void Dock::slideDownAnimUpdate() noexcept
{
    setMargin(SkIRect::MakeLTRB(0, 0, 0, -(layout().height().value - 2) * slideDownAnim.value()));
    update();
}

void Dock::slideDownAnimFinish() noexcept
{
    setMargin(SkIRect::MakeLTRB(0, 0, 0, -(layout().height().value - 2)));

    if (hover)
        showDock();
}

void Dock::layoutEvent(const AKLayoutEvent &e)
{
    MLayerSurface::layoutEvent(e);

    if (e.changes().check(AKLayoutEvent::Size | AKLayoutEvent::Scale))
    {
        SkRect side, center;
        background.setImage(Theme::dockTexture(Theme::MAX_DOCK_HEIGHT, scale(), side, center));
        background.setImageScale(scale());
        background.setSideSrcRect(side);
        background.setCenterSrcRect(center);
    }
}

void Dock::setupBackground() noexcept
{
    background.layout().setWidthPercent(100);
    background.layout().setHeightPercent(100);
}

void Dock::setupAnimations() noexcept
{
    slideUpAnim.setDuration(Theme::dockSlideUpMs);
    slideUpAnim.setOnUpdateCallback([this](AKAnimation*) { slideUpAnimUpdate(); });
    slideUpAnim.setOnFinishCallback([this](AKAnimation*) { slideUpAnimFinish(); });

    slideDownAnim.setDuration(Theme::dockSlideDownMs);
    slideDownAnim.setOnUpdateCallback([this](AKAnimation*) { slideDownAnimUpdate(); });
    slideDownAnim.setOnFinishCallback([this](AKAnimation*) { slideDownAnimFinish(); });
}
