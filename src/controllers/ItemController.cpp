#include <controllers/ItemController.h>
#include <core/Shelf.h>
#include <items/Item.h>

ItemController::ItemController(Type type) noexcept : type(type)
{
    for (Dock *dock : shelf().docks)
        new Item(*this, *dock);
}

ItemController::~ItemController() noexcept
{
    while (!items.empty())
        delete items.back();
}
