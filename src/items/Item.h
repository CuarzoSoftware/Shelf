#ifndef ITEM_H
#define ITEM_H

#include <include/core/SkRect.h>
#include <interfaces/Renderable.h>
#include <interfaces/PointerEventsListener.h>
#include <core/Object.h>

class ItemController;
class Dock;

class Item : public Object, public Renderable
{
public:
    Item(ItemController &controller, Dock &dock) noexcept;
    ~Item() noexcept;
    void paintEvent(SkCanvas *c) noexcept override;
    bool hasPointerFocus() const noexcept;

    SkRect rect;

    ItemController &controller;
    std::list<Item*>::iterator controllerLink;

    Dock &dock;
    std::list<Item*>::iterator dockLink;
};

#endif // ITEM_H
