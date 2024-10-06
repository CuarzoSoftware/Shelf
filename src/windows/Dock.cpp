#include <include/core/SkCanvas.h>
#include <include/core/SkMaskFilter.h>
#include <include/effects/SkGradientShader.h>
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
        zwlr_layer_surface_v1_ack_configure(dock.wlLayerSurface, serial);
        dock.resize(SkSize::Make(width, DOCK_SURFACE_HEIGHT));
        dock.updateDimensions();
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
    qInfo() << "New Dock!";

    createItems();

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

    zwlr_layer_surface_v1_set_exclusive_zone(wlLayerSurface, -1);
    zwlr_layer_surface_v1_set_size(wlLayerSurface, size().width(), 1);

    // TODO
    //wl_surface_set_input_region(wlSurface(), shelf().wlEmptyRegion);
    commit();

    const char *apps[] =
    {
        "nemo",
        "firefox",
        "google-chrome",
        "qtcreator",
        "spotify",
        "godot",
        "gimp",
        "inkscape",
        "gedit",
        "terminal",
    };

    for (int i = 0; i < sizeof(apps)/sizeof(*apps); i++)
        new AppController(false, apps[i], apps[i]);
}

Dock::~Dock() noexcept
{
    destroyItems();
    wl_output_destroy(wlOutput);
    Shelf::get().docks.erase(shelfLink);

    if (wlLayerSurface)
    {
        zwlr_layer_surface_v1_destroy(wlLayerSurface);
        wlLayerSurface = nullptr;
    }

    qInfo() << "Dock destroyed!";
}

void Dock::createItems() noexcept
{
    new Item(*shelf().separator, *this);
    new Item(*shelf().trash, *this);

    for (auto *app : shelf().pinnedApps)
        new Item(*app, *this);

    for (auto *app : shelf().unpinnedApps)
        new Item(*app, *this);
}

void Dock::destroyItems() noexcept
{
    while (!items.empty())
        delete items.back();
}

void Dock::updateDimensions() noexcept
{
    float n { DOCK_PADDING_LEFT + DOCK_PADDING_RIGHT + float(items.size() - 1) * ITEM_SPACING };

    for (Item *item : items)
        n += item->controller.scale;

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
    const float surfaceH { roundf(dockH + shadowRadius - shadowOffsetY + DOCK_MARGIN_T + DOCK_MARGIN_B + DOCK_MARGIN_B_EXTRA) };

    rect.setXYWH(
        (size().width() - dockW) / 2.f,
        surfaceH - DOCK_MARGIN_B - DOCK_MARGIN_B_EXTRA - dockH,
        dockW,
        dockH);

    const float halfSpacing { itemSpacing * 0.5f };
    float itemX { rect.x() + paddingL - halfSpacing};

    for (Item *item : items)
    {
        item->rect.fTop = rect.fTop;
        item->rect.fBottom = rect.fBottom;
        item->rect.fLeft = itemX;
        item->rect.fRight = itemX + itemSize * item->controller.scale + itemSpacing;
        itemX = item->rect.fRight;
    }

    zwlr_layer_surface_v1_set_size(wlLayerSurface, size().width(), surfaceH);
    resize(SkSize::Make(size().width(), surfaceH));
}

void Dock::beforePaintEvent() noexcept
{
    updateDimensions();
}

void Dock::paintEvent(SkCanvas *c) noexcept
{
    c->restore();
    c->resetMatrix();
    c->scale(scale(), scale());
    c->clear(SK_ColorTRANSPARENT);
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
    p.setColor(SkColorSetARGB(128, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(false);
    c->drawRoundRect(shadowRect, borderRadius, borderRadius, p);

    // Container fill

    SkColor4f colors[] = { SkColor4f::FromColor(SkColorSetARGB(253, 248, 244, 246)), SkColor4f::FromColor(SkColorSetARGB(250, 235, 233, 238)) };
    float positions[] = { 0.0f, 1.0f };  // Corresponding positions for the colors
    SkPoint points [] = {SkPoint::Make(rect.left(), rect.top()), SkPoint::Make(rect.left(), rect.bottom())};

    // Create a linear gradient shader
    sk_sp<SkShader> gradientShader = SkGradientShader::MakeLinear(points, colors, shelf().skColorSpace, positions, 2, SkTileMode::kClamp);
    p.setShader(gradientShader);
    p.setMaskFilter(nullptr);
    uint8_t color { 240 };
    p.setColor(SkColorSetARGB(250, color, color, color));
    p.setStyle(SkPaint::kFill_Style);
    p.setStroke(false);
    p.setAntiAlias(true);
    c->drawRoundRect(rect, borderRadius, borderRadius, p);

    // Container stroke
    p.setShader(nullptr);
    p.setAntiAlias(false);
    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(18, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(0.5f);
    c->drawRoundRect(rect, borderRadius, borderRadius, p);
    p.setAntiAlias(true);
    p.setColor(SkColorSetARGB(18, 0, 0, 0));
    p.setStrokeWidth(0.5f);
    c->drawRoundRect(rect, borderRadius, borderRadius, p);

    return;

    float x = 50;
    float y = 10;
    float w = 70;
    float h = 70;
    float borderRadius = 10;
    float caretX = 35;
    float caretSize = 14;
    float caretFactorH = 0.8f;
    float caretFactorY = 0.12f;

    SkPath path;

    // Caret begin
    path.moveTo(x + caretX - caretSize, y + h);
    path.cubicTo(
        x + caretX - caretSize / 2.0f, y + h + caretSize * caretFactorY,
        x + caretX - caretSize / 4.f, y + h + caretSize * caretFactorH,
        x + caretX, y + h + caretSize * caretFactorH);

    path.cubicTo(
        x + caretX + caretSize / 4.f, y + h + caretSize * caretFactorH,
        x + caretX + caretSize / 2.0f, y + h + caretSize * caretFactorY,
        x + caretX + caretSize, y + h);
    // Caret end

    path.lineTo(x + w - borderRadius, y + h); // B2
    path.quadTo(x + w, y + h, x + w, y + h - borderRadius); // BR
    path.lineTo(x + w, y + borderRadius); // R
    path.quadTo(x + w, y, x + w - borderRadius, y); // TR
    path.lineTo(x + borderRadius, y); // T
    path.quadTo(x, y, x, y + borderRadius); // TL
    path.lineTo(x, y + h - borderRadius); // L
    path.quadTo(x , y + h, x + borderRadius, y + h);
    path.close();

    blurFilter = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8 / 3.f);
    p.setMaskFilter(blurFilter);
    p.setBlendMode(SkBlendMode::kSrc);
    p.setColor(SkColorSetARGB(72, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(false);
    c->drawPath(path, p);

    p.setMaskFilter(nullptr);
    p.setColor(SkColorSetARGB(250, 250, 250, 250));
    p.setStyle(SkPaint::kFill_Style);
    p.setStroke(false);
    p.setAntiAlias(true);
    c->drawPath(path, p);

    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(36, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    c->drawPath(path, p);
}

void Dock::drawItems(SkCanvas *c) noexcept
{
    for (Item *item : items)
        item->paintEvent(c);
}

void Dock::onCallback(bool repainted) noexcept
{
    if (repainted)
    {
        update(false);

        /*
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        dockScale = 0.1f + 0.9f * (qSin(M_PI * 2.f * ts.tv_nsec/2000000000.f) + 1.f)/2.f;

        //for (Item *item : items())
        //    item->controller().setScale(0.1f + 0.9f * (qSin(M_PI * 2.f * ts.tv_nsec/1000000000.f) + 1.f)/2.f);
        updateDimensions();
        update();*/
    }
    else
    {
        if (sliding)
        {
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - slideAnimStart).count();

            if (diff >= slideAnimDurationMs)
            {
                sliding = false;
                setVisibility(toVisibility);
                commit();

                if (visibility == 1.f && !hasPointerFocus())
                    animateVisibility(1.f, 0.f, DOCK_SLIDE_ANIM_MS);
            }
            else
            {
                setVisibility(fromVisibility + ((toVisibility - fromVisibility)*diff)/slideAnimDurationMs);
                update(false);
            }
        }

        /*
        static int c { 0 };

        if (c % (60) == 0)
            new AppController();
        c++;
        update(false);*/
    }
}

void Dock::sizeChanged() noexcept
{
    setVisibility(visibility);
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
    fromVisibility = from;
    toVisibility = to;
    slideAnimDurationMs = ms;
    slideAnimStart = std::chrono::steady_clock::now();
    setVisibility(from);
    update(false);
}

void Dock::setVisibility(float v) noexcept
{
    visibility = v;
    const int32_t newY = DOCK_MARGIN_T * (1.f - v) - DOCK_MARGIN_B_EXTRA * v - size().height() * (1.f - v);

    if (newY != y)
    {
        y = newY;
        zwlr_layer_surface_v1_set_margin(wlLayerSurface, 0, 0, y , 0);
    }
}

