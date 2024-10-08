#include <controllers/ItemController.h>
#include <core/Shelf.h>
#include <items/Item.h>
#include <windows/Dock.h>
#include <QDebug>

Item::Item(ItemController &controller, Dock &dock) noexcept :
    controller(controller),
    dock(dock)
{
    controller.items.emplace_back(this);
    controllerLink = std::prev(controller.items.end());

    if (controller.type == ItemController::App)
    {
        qInfo() << "New App Item";
        dockLink = dock.items.insert(dock.separatorItem->dockLink, this);
    }

    else if (controller.type == ItemController::Separator)
    {
        qInfo() << "New Separator Item";
        dock.items.push_back(this);
        dockLink = std::prev(dock.items.end());
        dock.separatorItem = this;
    }

    else if (controller.type == ItemController::Trash)
    {
        qInfo() << "New Separator Item";
        dock.items.push_back(this);
        dockLink = std::prev(dock.items.end());
        dock.trashItem = this;
    }
}

Item::~Item() noexcept
{
    if (hasPointerFocus())
    {
        shelf().focusedItem = nullptr;
    }

    dock.items.erase(dockLink);
    controller.items.erase(controllerLink);
}

void Item::paintEvent(SkCanvas *c) noexcept
{
    controller.drawItem(c, *this);
}

bool Item::hasPointerFocus() const noexcept
{
    return shelf().focusedItem == this;
}
