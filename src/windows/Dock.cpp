#include <include/core/SkCanvas.h>
#include <include/core/SkMaskFilter.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <QDebug>
#include <QtMath>
#include <chrono>

#include <items/Item.h>
#include <controllers/AppController.h>
#include <windows/Dock.h>
#include <core/Constants.h>
#include <core/Shelf.h>

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
        dock.resize(SkSize::Make(width, DOCK_SURFACE_HEIGHT));
        //zwlr_layer_surface_v1_set_margin(dock.wlLayerSurface, 0, 0, (4 - dock.size().fHeight) * (1.f - dock.visibility) , 0);
        dock.setMapped(true);
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
        0, DOCK_SURFACE_HEIGHT);

    zwlr_layer_surface_v1_set_exclusive_zone(
        wlLayerSurface,
        -1);

    // TODO
    //wl_surface_set_input_region(wlSurface(), shelf().wlEmptyRegion);
    commit();

    for (int i = 0; i < 20; i++)
        new AppController();

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

void Dock::calculateDimensions() noexcept
{
    float n { DOCK_PADDING_LEFT + DOCK_PADDING_RIGHT + float(items().size() - 1) * ITEM_SPACING };

    for (Item *item : items())
        n += item->controller().scale();

    itemSize = dockScale * std::min(ITEM_MAX_SIZE, (size().width() - DOCK_MARGIN_L - DOCK_MARGIN_R)/n);

    itemSpacing = itemSize * ITEM_SPACING;
    dotMarginT = itemSize * ITEM_DOT_MARGIN_TOP;
    dotMarginB = itemSize * ITEM_DOT_MARGIN_BOTTOM;
    dotSize = itemSize * ITEM_DOT_SIZE;
    paddingT = itemSize * DOCK_PADDING_TOP;
    paddingL = itemSize * DOCK_PADDING_LEFT;
    paddingR = itemSize * DOCK_PADDING_RIGHT;
    shadowRadius = itemSize * DOCK_SHADOW_RADIUS;
    shadowOffsetY = itemSize * DOCK_SHADOW_OFFSET_Y;
    borderRadius = itemSize * DOCK_BORDER_RADIUS;

    const float dockW { itemSize * n };
    const float dockH { paddingT + itemSize + dotMarginT + dotMarginB + dotSize };
    const float surfaceH { roundf(dockH + shadowRadius - shadowOffsetY + DOCK_MARGIN_T + DOCK_MARGIN_B) };

    rect.setXYWH(
        (size().width() - dockW) / 2.f,
        surfaceH - DOCK_MARGIN_B - dockH,
        dockW,
        dockH);

    const float halfSpacing { itemSpacing * 0.5f };
    float itemX { rect.x() + paddingL - halfSpacing};

    for (Item *item : items())
    {
        item->rect().fTop = rect.fTop;
        item->rect().fBottom = rect.fBottom;
        item->rect().fLeft = itemX;
        item->rect().fRight = itemX + itemSize * item->controller().scale() + itemSpacing;
        itemX = item->rect().fRight;
    }

    if (!sliding)
        resize(SkSize::Make(size().width(), surfaceH));
}

void Dock::paintEvent(SkCanvas *c) noexcept
{
    c->restore();
    c->resetMatrix();
    c->scale(scale(), scale());
    c->clear(SK_ColorTRANSPARENT);

    //calculateDimensions();
    drawContainer(c);
    drawItems(c);
    c->flush();
}

void Dock::drawContainer(SkCanvas *c) noexcept
{
    SkPaint p;

    // Shadow
    const SkRect shadowRect = SkRect::MakeXYWH(
        rect.x(),
        rect.y() + shadowOffsetY,
        rect.width(),
        rect.height());
    sk_sp<SkMaskFilter> blurFilter = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, shadowRadius / 3.f);
    p.setMaskFilter(blurFilter);
    p.setBlendMode(SkBlendMode::kSrc);
    p.setColor(SkColorSetARGB(72, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(false);
    c->drawRoundRect(shadowRect, borderRadius, borderRadius, p);

    // Container fill
    p.setAntiAlias(true);
    p.setMaskFilter(nullptr);
    p.setColor(SkColorSetARGB(250, 250, 250, 250));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(true);
    c->drawRoundRect(rect, borderRadius, borderRadius, p);

    // Container stroke
    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(36, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(0.5f);
    c->drawRoundRect(rect, borderRadius, borderRadius, p);
}

void Dock::drawItems(SkCanvas *c) noexcept
{
    for (Item *item : items())
        item->paintEvent(c);
}

void Dock::onCallback(bool repainted) noexcept
{
    if (repainted)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        items().back()->controller().setScale(0.1f + 0.9f * (qSin(M_PI * 2.f * ts.tv_nsec/1000000000.f) + 1.f)/2.f);
        calculateDimensions();
        update(true);
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

                if (visibility == 1.f && !hasPointerFocus())
                    animateVisibility(1.f, 0.f, DOCK_SLIDE_ANIM_MS);
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

