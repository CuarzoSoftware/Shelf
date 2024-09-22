#ifndef SHELF_H
#define SHELF_H

#include <QHBoxLayout>
#include <DockContainer.h>

class QPropertyAnimation;

class Shelf final : public QWidget
{
public:
    Shelf() noexcept;
    void setDockVisible(bool visible) noexcept;
    void enterEvent(QEnterEvent *event) noexcept override;

    DockContainer *dockContainer { new DockContainer(this) };
    QPropertyAnimation *slideUpAnim, *slideDownAnim;
    bool isDockVisible { false };
};

#endif // SHELF_H
