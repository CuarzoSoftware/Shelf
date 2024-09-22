#ifndef DOCKCONTAINER_H
#define DOCKCONTAINER_H

#include <QWidget>
#include <QHBoxLayout>

class Shelf;
class Dock;

class DockContainer final : public QWidget
{
public:
    DockContainer(Shelf *shelf);
    QHBoxLayout *layout { new QHBoxLayout(this) };
    Shelf *shelf;
    Dock *dock;
    bool isMouseOver { false };
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
};

#endif // DOCKCONTAINER_H
