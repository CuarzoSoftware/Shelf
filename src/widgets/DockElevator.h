#ifndef DOCKELEVATOR_H
#define DOCKELEVATOR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPropertyAnimation>

class Dock;
class DockWidget;

class DockElevator final : public QWidget
{
public:
    DockElevator(Dock &dock);
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

    Dock &dock;
    bool isMouseOver { false };
    DockWidget *dockWidget;
    QHBoxLayout *layout { new QHBoxLayout(this) };
    QPropertyAnimation *slideUpAnim { new QPropertyAnimation(this, "pos", this) };
    QPropertyAnimation *slideDownAnim { new QPropertyAnimation(this, "pos", this) };
};

#endif // DOCKELEVATOR_H
