#include <DockContainer.h>
#include <Dock.h>
#include <Shelf.h>

DockContainer::DockContainer(Shelf *shelf) :
    shelf(shelf),
    dock(new Dock(shelf))
{
    setPalette(Qt::transparent);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_TranslucentBackground);

    layout->setContentsMargins(32, 32, 32, 6);
    layout->addWidget(dock);

    show();
    setFixedSize(200, 200);
    move(0, 2);
}

void DockContainer::enterEvent(QEnterEvent *)
{
    isMouseOver = true;
}

void DockContainer::leaveEvent(QEvent *)
{
    isMouseOver = false;
    shelf->setDockVisible(false);
}
