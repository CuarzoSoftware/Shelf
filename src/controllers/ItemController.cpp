#include <controllers/ItemController.h>
#include <core/Shelf.h>
#include <items/Item.h>

ItemController::ItemController(Type type, const QString &name) noexcept : type(type), name(name)
{
    for (Dock *dock : shelf().docks)
        new Item(*this, *dock);
}

ItemController::~ItemController() noexcept
{
    while (!items.empty())
        delete items.back();
}
