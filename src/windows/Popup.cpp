#include <windows/Popup.h>
#include <windows/Dock.h>
#include <protocols/xdg-shell.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <core/Shelf.h>
#include <include/core/SkCanvas.h>

static const xdg_popup_listener xdgPopupListener
{
    .configure = [](void *data, xdg_popup *xdgPopup, int32_t x, int32_t y, int32_t width, int32_t height)
    {

    },
    .popup_done = [](void *data, xdg_popup *xdgPopup)
    {
        Popup *popup = static_cast<Popup*>(data);
        popup->setMapped(false);
        delete popup;
    }
};

static const xdg_surface_listener xdgSurfaceListener
{
    .configure = [](void *data, xdg_surface *xdgSurface, uint32_t serial)
    {
        xdg_surface_ack_configure(xdgSurface, serial);
        Popup *popup = static_cast<Popup*>(data);
        popup->setMapped(true);
    }
};

Popup::Popup(Dock &dock, const SkPoint &pos, const std::vector<MenuItem> &items) noexcept :
    Window(Type::Popup),
    dock(dock),
    pos(pos),
    items(items)
{

    if (shelf().popup)
        delete shelf().popup;

    updateDimensions();

    xdgSurface = xdg_wm_base_get_xdg_surface(shelf().xdgWmBase, wlSurface());
    xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, this);

    xdg_positioner *xdgPositioner = xdg_wm_base_create_positioner(shelf().xdgWmBase);
    xdg_positioner_set_anchor(xdgPositioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
    xdg_positioner_set_gravity(xdgPositioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
    xdg_positioner_set_size(xdgPositioner, size().width(), size().height());
    xdg_positioner_set_anchor_rect(xdgPositioner, 0, 0, 1, 1);
    xdg_positioner_set_offset(xdgPositioner, surfacePos.x(), surfacePos.y());
    xdgPopup = xdg_surface_get_popup(xdgSurface, NULL, xdgPositioner);
    xdg_popup_add_listener(xdgPopup, &xdgPopupListener, this);
    zwlr_layer_surface_v1_get_popup(dock.wlLayerSurface, xdgPopup);
    commit();
    xdg_positioner_destroy(xdgPositioner);

    shelf().popup = this;
}

Popup::~Popup() noexcept
{
    if (shelf().popup == this)
        shelf().popup = nullptr;

    xdg_popup_destroy(xdgPopup);
    xdg_surface_destroy(xdgSurface);

    if (shelf().focusedWindow != &dock)
        dock.animateVisibility(1.f, 0.f, DOCK_SLIDE_ANIM_MS);
}

bool Popup::updateDimensions() noexcept
{
    bool needsRepaint { false };
    const SkISize prevSurfaceSize { SkISize::Make(size().width(), size().height()) };

    float greatestItemWidth { 0.f };

    containerSize.fHeight = 2.f * MENU_PADDING;

    for (MenuItem &item : items)
    {
        shelf().tooltipFont.measureText(item.text.toUtf8().constData(), item.text.length(), SkTextEncoding::kUTF8, &item.textBounds);

        if (item.textBounds.width() > greatestItemWidth)
            greatestItemWidth = item.textBounds.width();

        containerSize.fHeight += shelf().tooltipFont.getSize();
    }

    containerSize.fWidth = greatestItemWidth + 2.f * MENU_PADDING;

    const SkISize newSurfaceSize { SkISize::Make(
        2.f * MENU_BLUR_RADIUS + containerSize.width(),
        2.f * MENU_BLUR_RADIUS + containerSize.height() + MENU_CARET_SIZE) };

    /* Surface pos */

    surfacePos.fY = pos.y() - newSurfaceSize.height();
    surfacePos.fX = pos.x() - newSurfaceSize.width() / 2.f;

    if (surfacePos.fX < 0.f)
        surfacePos.fX = 0;
    else if (surfacePos.fX + newSurfaceSize.width() > dock.size().width())
        surfacePos.fX = dock.size().width() - newSurfaceSize.width();

    /* Caret x pos */
    const int prevCaretPosX { int(caretX) };
    caretX = pos.x() - surfacePos.x() - MENU_BLUR_RADIUS;

    if (int(caretX) != prevCaretPosX)
        needsRepaint = true;

    if (prevSurfaceSize != newSurfaceSize)
    {
        needsRepaint = true;
        resize(SkSize::Make(newSurfaceSize.width(), newSurfaceSize.height()));
    }

    return needsRepaint;
}

void Popup::paintEvent(SkCanvas *c) noexcept
{
    c->resetMatrix();
    c->restore();
    c->scale(scale(), scale());
    c->clear(SK_ColorTRANSPARENT);

    const float x { MENU_BLUR_RADIUS };
    const float y { MENU_BLUR_RADIUS };
    const float w { containerSize.width() };
    const float h { containerSize.height() };
    const float caretFactorH { 0.8f };
    const float caretFactorY { 0.12f };

    SkPaint p;
    SkPath path;

    // Caret begin
    path.moveTo(x + caretX - MENU_CARET_SIZE, y + h);
    path.cubicTo(
        x + caretX - MENU_CARET_SIZE / 2.0f, y + h + MENU_CARET_SIZE * caretFactorY,
        x + caretX - MENU_CARET_SIZE / 4.f, y + h + MENU_CARET_SIZE * caretFactorH,
        x + caretX, y + h + MENU_CARET_SIZE * caretFactorH);

    path.cubicTo(
        x + caretX + MENU_CARET_SIZE / 4.f, y + h + MENU_CARET_SIZE * caretFactorH,
        x + caretX + MENU_CARET_SIZE / 2.0f, y + h + MENU_CARET_SIZE * caretFactorY,
        x + caretX + MENU_CARET_SIZE, y + h);
    // Caret end

    path.lineTo(x + w - MENU_BORDER_RADIUS, y + h); // B2
    path.quadTo(x + w, y + h, x + w, y + h - MENU_BORDER_RADIUS); // BR
    path.lineTo(x + w, y + MENU_BORDER_RADIUS); // R
    path.quadTo(x + w, y, x + w - MENU_BORDER_RADIUS, y); // TR
    path.lineTo(x + MENU_BORDER_RADIUS, y); // T
    path.quadTo(x, y, x, y + MENU_BORDER_RADIUS); // TL
    path.lineTo(x, y + h - MENU_BORDER_RADIUS); // L
    path.quadTo(x , y + h, x + MENU_BORDER_RADIUS, y + h);
    path.close();

    // Blur
    p.setMaskFilter(shelf().tooltipBlurMaskFilter);
    p.setBlendMode(SkBlendMode::kSrc);
    p.setColor(SkColorSetARGB(56, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(false);
    c->drawPath(path, p);

    // Fill
    p.setMaskFilter(nullptr);
    const uint8_t dockAvgColor[3] {
        uint8_t(255.f * (shelf().dockGradientColors[0].fR + shelf().dockGradientColors[1].fR) * 0.5f),
        uint8_t(255.f * (shelf().dockGradientColors[0].fG + shelf().dockGradientColors[1].fG) * 0.5f),
        uint8_t(255.f * (shelf().dockGradientColors[0].fB + shelf().dockGradientColors[1].fB) * 0.5f)
    };
    p.setColor(SkColorSetARGB(248, dockAvgColor[0], dockAvgColor[1], dockAvgColor[2]));
    p.setStyle(SkPaint::kFill_Style);
    p.setStroke(false);
    p.setAntiAlias(true);
    c->drawPath(path, p);

    // Stroke white
    p.setStroke(true);
    p.setStrokeWidth(1.f);
    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(24, 255, 255, 255));
    p.setStyle(SkPaint::kStroke_Style);
    c->drawPath(path, p);

    // Stroke round
    p.setStroke(true);
    p.setStrokeWidth(0.45f);
    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(42, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    c->drawPath(path, p);

    // Stroke lines
    p.setAntiAlias(false);
    p.setStrokeWidth(0.5f);
    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(12, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    c->drawPath(path, p);

    c->restore();

    // Text
    p.reset();
    p.setAntiAlias(true);

    if ((dockAvgColor[0] + dockAvgColor[1] + dockAvgColor[2]) / 3 > 126)
        p.setColor(SkColorSetARGB(229, 0, 0, 0));
    else
        p.setColor(SkColorSetARGB(229, 255, 255, 255));

    p.setBlendMode(SkBlendMode::kSrc);

    /*
    SkFontMetrics m;
    shelf().tooltipFont.getMetrics(&m);

    c->drawString(
        text.toUtf8().constData(),
        MENU_BLUR_RADIUS + (containerSize.width() - textBounds.width()) / 2.f,
        MENU_BLUR_RADIUS + (containerSize.height() - shelf().tooltipFont.getSize()) / 2.f + (shelf().tooltipFont.getSize() -  m.fDescent * 0.65f),
        shelf().tooltipFont, p);
    */
    c->flush();
}
