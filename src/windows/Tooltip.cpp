#include <windows/Tooltip.h>
#include <windows/Dock.h>
#include <core/Shelf.h>
#include <core/Constants.h>
#include <items/Item.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>

Tooltip::Tooltip(Dock &dock) noexcept :
    Window(Type::Tooltip),
    dock(dock)
{
    wlSubsurface = wl_subcompositor_get_subsurface(shelf().wlSubcompositor, wlSurface(), dock.wlSurface());
    wl_surface_set_input_region(wlSurface(), shelf().wlEmptyRegion);
}

Tooltip::~Tooltip() noexcept
{
    if (wlSubsurface)
    {
        wl_subsurface_destroy(wlSubsurface);
        wlSubsurface = nullptr;
    }
}

void Tooltip::beforePaintEvent() noexcept
{

}

void Tooltip::afterPaintEvent() noexcept
{
    dock.commit();
}

void Tooltip::paintEvent(SkCanvas *c) noexcept
{
    c->resetMatrix();
    c->restore();
    c->scale(scale(), scale());
    c->clear(SK_ColorTRANSPARENT);

    const float x { TOOLTIP_BLUR_RADIUS };
    const float y { TOOLTIP_BLUR_RADIUS };
    const float w { containerSize.width() };
    const float h { containerSize.height() };
    const float caretFactorH { 0.8f };
    const float caretFactorY { 0.12f };

    SkPaint p;
    SkPath path;

    // Caret begin
    path.moveTo(x + caretX - TOOLTIP_CARET_SIZE, y + h);
    path.cubicTo(
        x + caretX - TOOLTIP_CARET_SIZE / 2.0f, y + h + TOOLTIP_CARET_SIZE * caretFactorY,
        x + caretX - TOOLTIP_CARET_SIZE / 4.f, y + h + TOOLTIP_CARET_SIZE * caretFactorH,
        x + caretX, y + h + TOOLTIP_CARET_SIZE * caretFactorH);

    path.cubicTo(
        x + caretX + TOOLTIP_CARET_SIZE / 4.f, y + h + TOOLTIP_CARET_SIZE * caretFactorH,
        x + caretX + TOOLTIP_CARET_SIZE / 2.0f, y + h + TOOLTIP_CARET_SIZE * caretFactorY,
        x + caretX + TOOLTIP_CARET_SIZE, y + h);
    // Caret end

    path.lineTo(x + w - TOOLTIP_BORDER_RADIUS, y + h); // B2
    path.quadTo(x + w, y + h, x + w, y + h - TOOLTIP_BORDER_RADIUS); // BR
    path.lineTo(x + w, y + TOOLTIP_BORDER_RADIUS); // R
    path.quadTo(x + w, y, x + w - TOOLTIP_BORDER_RADIUS, y); // TR
    path.lineTo(x + TOOLTIP_BORDER_RADIUS, y); // T
    path.quadTo(x, y, x, y + TOOLTIP_BORDER_RADIUS); // TL
    path.lineTo(x, y + h - TOOLTIP_BORDER_RADIUS); // L
    path.quadTo(x , y + h, x + TOOLTIP_BORDER_RADIUS, y + h);
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

    SkFontMetrics m;
    shelf().tooltipFont.getMetrics(&m);

    c->drawString(
        text.toUtf8().constData(),
        TOOLTIP_BLUR_RADIUS + (containerSize.width() - textBounds.width()) / 2.f,
        TOOLTIP_BLUR_RADIUS + (containerSize.height() - shelf().tooltipFont.getSize()) / 2.f + (shelf().tooltipFont.getSize() -  m.fDescent * 0.65f),
        shelf().tooltipFont, p);

    c->flush();
}

bool Tooltip::updateDimensions() noexcept
{
    bool needsRepaint { false };
    const SkISize prevSurfaceSize { SkISize::Make(size().width(), size().height()) };

    shelf().tooltipFont.measureText(text.toUtf8().constData(), text.length(), SkTextEncoding::kUTF8, &textBounds);

    containerSize.fWidth = 3.f * TOOLTIP_PADDING + textBounds.width();
    containerSize.fHeight = 2.f * TOOLTIP_PADDING + shelf().tooltipFont.getSize();

    const SkISize newSurfaceSize { SkISize::Make(
        2.f * TOOLTIP_BLUR_RADIUS + containerSize.width(),
        2.f * TOOLTIP_BLUR_RADIUS + containerSize.height() + TOOLTIP_CARET_SIZE) };

    /* Surface pos */

    surfacePos.fY = pos.y() - newSurfaceSize.height();
    surfacePos.fX = pos.x() - newSurfaceSize.width() / 2.f;

    if (surfacePos.fX < 0.f)
        surfacePos.fX = 0;
    else if (surfacePos.fX + newSurfaceSize.width() > dock.size().width())
        surfacePos.fX = dock.size().width() - newSurfaceSize.width();

    /* Caret x pos */
    const int prevCaretPosX { int(caretX) };
    caretX = pos.x() - surfacePos.x() - TOOLTIP_BLUR_RADIUS;

    if (int(caretX) != prevCaretPosX)
        needsRepaint = true;

    if (prevSurfaceSize != newSurfaceSize)
    {
        needsRepaint = true;
        resize(SkSize::Make(newSurfaceSize.width(), newSurfaceSize.height()));
    }

    return needsRepaint;
}

void Tooltip::show(float x, float y, const QString &text) noexcept
{
    if (dock.visibility != 1.f)
    {
        setMapped(false);
        return;
    }

    bool needsRepaint { !mapped() };
    const SkIPoint prevSurfacePos { surfacePos };
    pos.set(x, y);

    if (this->text != text)
    {
        needsRepaint = true;
        this->text = text;
    }

    if (text.isEmpty())
    {
        setMapped(false);
        dock.commit();
        return;
    }

    if (updateDimensions())
        needsRepaint = true;

    const bool updatePos { prevSurfacePos != surfacePos };

    if (updatePos)
        wl_subsurface_set_position(wlSubsurface, surfacePos.x(), surfacePos.y());

    if (updatePos || needsRepaint)
        setMapped(true);

    if (needsRepaint)
    {
        update();
        commit();
    }

    if (updatePos || needsRepaint)
    {
        dock.update(false);
    }
}

void Tooltip::showItem(Item *item) noexcept
{
    if (item)
    {
        show(item->rect.x() + item->rect.width() / 2.f,
             dock.rect.y() + TOOLTIP_BLUR_RADIUS - 0.12f * dock.itemSize,
             item->controller.name);
    }
    else
    {
        setMapped(false);
        dock.commit();
    }
}
