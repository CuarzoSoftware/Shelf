#include <controllers/AppController.h>
#include <include/core/SkCanvas.h>
#include <include/effects/SkImageFilters.h>
#include <items/Item.h>
#include <QPixmap>
#include <windows/Dock.h>
#include <core/Utils.h>
#include <core/Constants.h>
#include <core/Shelf.h>

AppController::AppController(bool running, const QString &name, const QString &iconName) noexcept :
    ItemController(App),
    running(running),
    name(name),
    iconName(iconName)
{
    qInfo() << "New App Controller";

    QPixmap pixmap = Utils::iconFromName(iconName, ICON_SIZE);

    if (pixmap.isNull())
        icon = shelf().defaultIcon;
    else
        icon = Utils::skImageFromPixmap(Utils::iconApplyShadow(pixmap, ICON_SIZE));

    for (Dock *dock : shelf().docks)
        dock->update();
}

void AppController::drawItem(SkCanvas *c, Item &item) noexcept
{
    c->restore();
    static const SkSamplingOptions linear { SkFilterMode::kLinear };
    const float size { item.dock.itemSize * scale };
    const float iconSpaceH { item.dock.rect.height() - item.dock.dotMarginB - item.dock.dotMarginT - item.dock.dotSize };
    const SkRect rect { SkRect::MakeXYWH(
        item.rect.x() + (item.rect.width() - size) / 2.f,
        item.dock.rect.y() + (iconSpaceH - size) / 2.f,
        size, size) };

    sk_sp<SkImageFilter> blurFilter = SkImageFilters::DropShadow(0.f, 1.f,
                                                                 0.5f, 1.f,
                                                                 SkColorSetARGB(64, 0, 0, 0), nullptr);

    SkPaint p;
    p.setAntiAlias(false);
    //p.setImageFilter(blurFilter);
    c->drawImageRect(icon, rect,linear, &p);

    p.setImageFilter(nullptr);
    p.setAntiAlias(true);
    p.setColor(SkColorSetARGB(150, 0, 0, 0));
    c->drawCircle(
        item.rect.x() + item.rect.width() / 2.f,
        item.dock.rect.fBottom - item.dock.dotMarginB - item.dock.dotSize / 2.f,
        item.dock.dotSize / 2.f,
        p);
}
