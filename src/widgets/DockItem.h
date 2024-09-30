#ifndef DOCKITEM_H
#define DOCKITEM_H

#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <widgets/Tooltip.h>
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
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

    Tooltip *tooltip { new Tooltip(nullptr) };

    QVBoxLayout *layout { new QVBoxLayout(this) };
    QLabel *icon = new QLabel(this);
    QGraphicsDropShadowEffect *shadow { new QGraphicsDropShadowEffect(icon)};
    QLabel *dot { new QLabel(this) };

    Dock &dock;
    ItemController &controller;
    std::list<DockItem*>::iterator controllerLink;
};

#endif // DOCKITEM_H
