#ifndef DOCKITEM_H
#define DOCKITEM_H

#include <QGraphicsColorizeEffect>
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

    bool mouseIsOver { false };
    void updateDimensions() noexcept;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    QGraphicsColorizeEffect *darkFx { new QGraphicsColorizeEffect(this) };

    Tooltip *tooltip { new Tooltip(nullptr) };

    QVBoxLayout *layout { new QVBoxLayout(this) };
    QWidget *jumpingIconContainer { new QWidget(this) };
    QLabel *jumpingIcon = new QLabel(jumpingIconContainer);

    QWidget *iconSubstitute { new QWidget(this) };
    QLabel *icon = new QLabel(this);
    QGraphicsDropShadowEffect *shadow { new QGraphicsDropShadowEffect(icon)};
    QGraphicsDropShadowEffect *shadowJump { new QGraphicsDropShadowEffect(jumpingIcon)};
    QLabel *dot { new QLabel(this) };

    Dock &dock;
    ItemController &controller;
    std::list<DockItem*>::iterator controllerLink;
};

#endif // DOCKITEM_H
