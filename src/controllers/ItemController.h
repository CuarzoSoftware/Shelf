#ifndef ITEMCONTROLLER_H
#define ITEMCONTROLLER_H

#include <core/Object.h>
#include <list>

class Item;
class SkCanvas;
class Dock;

class ItemController : public Object
{
public:
    enum Type
    {
        App,
        Dir,
        Thumbnail,
        Trash,
        Separator
    };

    ItemController(Type type) noexcept;
    virtual ~ItemController() noexcept;

    virtual void drawItem(SkCanvas *c, Item &item) noexcept = 0;

    std::list<Item*> items;
    float scale { 1.f };
    Type type;
};

#endif // ITEMCONTROLLER_H
