#include <widgets/DockElevator.h>
#include <widgets/DockWidget.h>
#include <widgets/Dock.h>
#include <controller/Shelf.h>

DockElevator::DockElevator(Dock &dock) :
    dock(dock),
    dockWidget(new DockWidget(dock))
{
    slideUpAnim->setDuration(200);
    slideDownAnim->setDuration(200);

    connect(slideUpAnim, &QPropertyAnimation::finished, [this]()
    {
        if (!this->dock.isDockVisible)
            this->dock.setDockVisible(false);
    });

    connect(slideDownAnim, &QPropertyAnimation::finished,[this]()
    {
        if (this->dock.isDockVisible)
            this->dock.setDockVisible(true);
        else
            hide();
    });

    Shelf::makeWidgetTransparent(this);

    layout->setContentsMargins(32, 32, 32, 6);

    layout->addStretch(1);
    layout->addWidget(dockWidget);
    layout->addStretch(1);

    show();
    setFixedSize(200, 200);
    move(0, 2);
}

void DockElevator::enterEvent(QEnterEvent *)
{
    isMouseOver = true;
}

void DockElevator::leaveEvent(QEvent *)
{
    isMouseOver = false;
    dock.setDockVisible(false);
}
