#ifndef ITEMCONTROLLER_H
#define ITEMCONTROLLER_H

#include <interfaces/PointerEventsListener.h>
#include <core/Object.h>
#include <list>
#include <QString>

class Item;
class SkCanvas;
class Dock;

class ItemController : public Object, public PointerEventsListener
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

    ItemController(Type type, const QString &name) noexcept;
    virtual ~ItemController() noexcept;

    virtual void drawItem(SkCanvas *c, Item &item) noexcept = 0;

    std::list<Item*> items;
    float scale { 1.f };
    Type type;
    QString name;
};

#endif // ITEMCONTROLLER_H
