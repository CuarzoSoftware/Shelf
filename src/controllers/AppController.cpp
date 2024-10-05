#include <controllers/AppController.h>
#include <include/core/SkCanvas.h>
#include <items/Item.h>
#include <QPixmap>
#include <windows/Dock.h>

AppController::AppController() noexcept :
    ItemController(App, false, "App", QPixmap(), "AppIcon")
{

}

void AppController::drawItem(SkCanvas *c, Item &item) noexcept
{
    c->restore();
    static const SkSamplingOptions linear { SkFilterMode::kLinear };
    static SkPaint p;
    p.setAntiAlias(true);

    const float size { item.dock().itemSize * scale() };
    const float iconSpaceH { item.dock().rect.height() - item.dock().dotMarginB - item.dock().dotMarginT - item.dock().dotSize };

    c->drawImageRect(
        icon(),
        SkRect::MakeXYWH(
            item.rect().x() + (item.rect().width() - size) / 2.f,
            item.dock().rect.y() + (iconSpaceH - size) / 2.f,
            size, size),
        linear,
        &p);
}
