#include <include/core/SkCanvas.h>
#include <include/core/SkMaskFilter.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <Shelf.h>
#include <Dock.h>
#include <QDebug>
#include <QtMath>
#include <chrono>
#include <Constants.h>

static const zwlr_layer_surface_v1_listener wlLayerSurfaceListener
{
    .configure = [](void *data, zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial, uint32_t width, uint32_t height)
    {
        if (width == 0)
        {
            // TODO: Use xdg_output as fallback
            qFatal() << "Compositor configured dock with width = 0. Could not determine screen size.";
            exit(1);
        }

        Dock &dock { *static_cast<Dock*>(data) };
        dock.resize(SkSize::Make(width, height));
        dock.setMapped(true);
        dock.animateVisibility(1.f, 0.f, 300);
    },

    .closed = [](void *data, zwlr_layer_surface_v1 *zwlr_layer_surface_v1)
    {

    }
};

Dock::Dock(wl_output *wlOutput, uint32_t name) noexcept :
    Window(Type::Dock),
    wlOutput(wlOutput),
    name(name)
{
    wl_output_set_user_data(wlOutput, this);
    Shelf::get().docks.emplace_back(this);
    shelfLink = std::prev(Shelf::get().docks.end());

    wlLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
        Shelf::get().wlLayerShell,
        wlSurface(),
        wlOutput,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        "cuarzo-shelf");

    zwlr_layer_surface_v1_add_listener(wlLayerSurface, &wlLayerSurfaceListener, this);

    zwlr_layer_surface_v1_set_anchor(
        wlLayerSurface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

    zwlr_layer_surface_v1_set_size(
        wlLayerSurface,
        0, 100);

    zwlr_layer_surface_v1_set_exclusive_zone(
        wlLayerSurface,
        -1);

    // TODO
    //wl_surface_set_input_region(wlSurface(), shelf().wlEmptyRegion);
    commit();

    qInfo() << "New Dock!";
}

Dock::~Dock() noexcept
{
    wl_output_destroy(wlOutput);
    Shelf::get().docks.erase(shelfLink);

    if (wlLayerSurface)
    {
        zwlr_layer_surface_v1_destroy(wlLayerSurface);
        wlLayerSurface = nullptr;
    }

    qInfo() << "Dock destroyed!";
}

void Dock::paintEvent(SkCanvas *c) noexcept
{
    c->save();
    c->clear(SK_ColorTRANSPARENT);
    c->scale(scale(), scale());

    static float phase { 0.0 };
    phase += 0.05;

    const SkRect rect { SkRect::MakeXYWH(20, 20, size().width() - 40, 96) };

    SkPaint p;
    SkScalar blurRadius = 10.0f;
    sk_sp<SkMaskFilter> blurFilter = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blurRadius);
    p.setMaskFilter(blurFilter);
    p.setBlendMode(SkBlendMode::kSrc);

    p.setColor(SkColorSetARGB(96, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(true);
    c->drawRoundRect(SkRect::MakeXYWH(20, 25, size().width() - 40, 96), 20, 20, p);

    p.setMaskFilter(nullptr);
    p.setColor(SkColorSetARGB(250, 255, 255, 255));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(true);
    c->drawRoundRect(rect, 20, 20, p);

    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(96, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(0.5f);
    c->drawRoundRect(rect, 20, 20, p);

    p.setStyle(SkPaint::kFill_Style);
    p.setColor(SkColorSetARGB(96, 255, 0, 0));
    c->drawCircle(size().width()/2.f - qSin(phase)* (size().width()/2), 40, 10 , p);

    for (double i = 0.0; i < 20; i++)
        c->drawImageRect(Shelf::get().defaultIcon, SkRect::MakeXYWH(i * 72 + qSin(phase) * 100.f, 20, 64, 64), SkSamplingOptions(SkFilterMode::kLinear));

    c->flush();
    c->restore();
}

void Dock::onCallback(bool repainted) noexcept
{
    if (repainted)
    {

    }
    else
    {
        if (sliding)
        {
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - slideAnimStart).count();

            if (diff >= slideAnimDurationMs)
            {
                visibility = toVisibility;
                sliding = false;
                zwlr_layer_surface_v1_set_margin(wlLayerSurface, 0, 0, (4 - size().fHeight) * (1.f - visibility) , 0);
                commit();
            }
            else
            {
                visibility = fromVisibility + ((toVisibility - fromVisibility)*diff)/slideAnimDurationMs;
                zwlr_layer_surface_v1_set_margin(wlLayerSurface, 0, 0, (4 - size().fHeight) * (1.f - visibility) , 0);
                update(false);
            }
        }
    }
}

void Dock::pointerEnterEvent() noexcept
{
    if (!sliding && visibility != 1.f)
        animateVisibility(0.f, 1.f, DOCK_SLIDE_ANIM_MS);
}

void Dock::pointerLeaveEvent() noexcept
{
    if (!sliding && visibility != 0.f)
        animateVisibility(1.f, 0.f, DOCK_SLIDE_ANIM_MS);
}

void Dock::animateVisibility(float from, float to, int64_t ms) noexcept
{
    sliding = true;
    visibility = from;
    fromVisibility = from;
    toVisibility = to;
    slideAnimDurationMs = ms;
    slideAnimStart = std::chrono::steady_clock::now();
    zwlr_layer_surface_v1_set_margin(wlLayerSurface, 0, 0, (4 - size().fHeight) * (1.f - visibility) , 0);
    update(false);
}

