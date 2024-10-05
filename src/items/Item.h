#ifndef ITEM_H
#define ITEM_H

#include <include/core/SkRect.h>
#include <interfaces/Renderable.h>
#include <core/Object.h>

class ItemController;
class Dock;

class Item : public Object, public Renderable
{
public:
    Item(ItemController &controller, Dock &dock) noexcept;
    ~Item() noexcept;

    ItemController &controller() const noexcept { return m_controller; }
    Dock &dock() const noexcept { return m_dock; }
    SkRect &rect() noexcept { return m_rect; }

    void paintEvent(SkCanvas *c) noexcept override;

private:
    SkRect m_rect;

    ItemController &m_controller;
    std::list<Item*>::iterator m_controllerLink;

    Dock &m_dock;
    std::list<Item*>::iterator m_dockLink;
};

#endif // ITEM_H
