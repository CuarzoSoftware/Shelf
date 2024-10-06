#include <controllers/TrashController.h>
#include <include/core/SkCanvas.h>
#include <include/effects/SkImageFilters.h>
#include <items/Item.h>
#include <QPixmap>
#include <windows/Dock.h>
#include <core/Utils.h>
#include <core/Constants.h>
#include <core/Shelf.h>
#include <QDebug>

TrashController::TrashController() noexcept : ItemController(Trash)
{
    qInfo() << "New Trash Controller";
}

void TrashController::drawItem(SkCanvas *c, Item &item) noexcept
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
    p.setImageFilter(blurFilter);
    c->drawImageRect(shelf().trashFullIcon, rect, linear, &p);
}
