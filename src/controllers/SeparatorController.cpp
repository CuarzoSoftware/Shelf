#include <include/core/SkCanvas.h>
#include <controllers/SeparatorController.h>
#include <core/Constants.h>
#include <items/Item.h>
#include <windows/Dock.h>
#include <core/Shelf.h>
#include <linux/input-event-codes.h>

SeparatorController::SeparatorController() noexcept :
    ItemController(Separator, "")
{
    scale = SEPARATOR_SCALE;
}

void SeparatorController::drawItem(SkCanvas *c, Item &item) noexcept
{
    SkPaint p;

    const float dockColorAvg { (shelf().dockGradientColors[1].fR + shelf().dockGradientColors[1].fG + shelf().dockGradientColors[1].fB) / 3.f };

    if (dockColorAvg > 0.5f)
        p.setColor(SkColorSetARGB(72, 64, 64, 64));
    else
        p.setColor(SkColorSetARGB(72, 255, 255, 255));

    p.setStrokeWidth(1);

    const float xMid { item.rect.x() + item.rect.width() / 2.f };

    c->drawLine(
        xMid, item.rect.y() + item.dock.paddingT,
        xMid, item.rect.y() + item.rect.height() - item.dock.paddingT,
        p);
}

void SeparatorController::pointerEnterEvent() noexcept
{
    Shelf::setCursor(CursorNSResize);
    Tooltip &tooltip { shelf().focusedItem->dock.tooltip };
    tooltip.showItem(nullptr);
}

void SeparatorController::pointerLeaveEvent() noexcept
{
    Shelf::setCursor(CursorArrow);
}

void SeparatorController::pointerMotionEvent() noexcept
{
    if (shelf().draggedItem)
    {
        const float diffX { shelf().pointerPos.x() - dragInitPos.x() };
        shelf().draggedItem->dock.dockScale = dragInitDockScale + 0.01f * diffX;

        if (shelf().draggedItem->dock.dockScale > 1.f)
            shelf().draggedItem->dock.dockScale = 1.f;
        else if (shelf().draggedItem->dock.dockScale < 0.25f)
            shelf().draggedItem->dock.dockScale = 0.25f;

        shelf().draggedItem->dock.update();
    }
}

void SeparatorController::pointerButtonEvent() noexcept
{
    if (shelf().pointerButton == BTN_LEFT && shelf().pointerButtonState == WL_POINTER_BUTTON_STATE_PRESSED)
    {
        dragInitPos = shelf().pointerPos;
        dragInitDockScale = shelf().focusedItem->dock.dockScale;
    }
}
