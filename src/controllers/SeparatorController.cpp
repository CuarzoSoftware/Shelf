#include <include/core/SkCanvas.h>
#include <controllers/SeparatorController.h>
#include <core/Constants.h>
#include <items/Item.h>
#include <windows/Dock.h>
#include <core/Shelf.h>

SeparatorController::SeparatorController() noexcept :
    ItemController(Separator, "")
{
    qInfo() << "New Separator Controller";

    scale = SEPARATOR_SCALE;
}

void SeparatorController::drawItem(SkCanvas *c, Item &item) noexcept
{
    SkPaint p;
    p.setColor(SkColorSetARGB(72, 64, 64, 64));
    p.setStrokeWidth(1);

    const float xMid { item.rect.x() + item.rect.width() / 2.f };

    c->drawLine(
        xMid, item.rect.y() + item.dock.paddingT,
        xMid, item.rect.y() + item.rect.height() - item.dock.paddingT,
        p);
}

void SeparatorController::pointerEnterEvent() noexcept
{
    Tooltip &tooltip { shelf().focusedItem->dock.tooltip };
    tooltip.showItem(nullptr);
}
