#ifndef DOCKITEM_H
#define DOCKITEM_H

#include <QGraphicsColorizeEffect>
#include <QVBoxLayout>
#include <widgets/Tooltip.h>
#include <widgets/Label.h>
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

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool mouseIsOver { false };

    Tooltip tooltip { nullptr };
    QVBoxLayout layout { this };

    /* Ideally, "icon" should be reused and turned into a subsurface when jumping, but the change is racy
     * and doing so causes 1 frame of the icon disappearing. */
    Label jumpingIcon { this };

    QLabel icon { this };
    QLabel dot { this };

    QGraphicsColorizeEffect darkFx { this };

    /* Used to fill the dock space while jumping */
    QWidget iconSubstitute { this };

    Dock &dock;
    ItemController &controller;
    std::list<DockItem*>::iterator controllerLink;
};

#endif // DOCKITEM_H
