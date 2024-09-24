#ifndef DOCKITEM_H
#define DOCKITEM_H

#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <list>

class ItemController;
class Dock;

class DockItem : public QWidget
{
public:
    DockItem(ItemController &controller, Dock &dock) noexcept;
    ~DockItem() noexcept;

    void updateDimensions() noexcept;

    QLabel *icon = new QLabel(this);
    QGraphicsDropShadowEffect *shadow { new QGraphicsDropShadowEffect(icon)};

    Dock &dock;
    ItemController &controller;
    std::list<DockItem*>::iterator controllerLink;
};

#endif // DOCKITEM_H
