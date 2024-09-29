#include <controller/ItemController.h>
#include <widgets/DockElevator.h>
#include <widgets/DockItem.h>
#include <widgets/DockWidget.h>
#include <widgets/Dock.h>
#include <controller/Shelf.h>
#include <QWindow>
#include <QTimer>

DockElevator::DockElevator(Dock &dock) :
    dock(dock),
    dockWidget(new DockWidget(dock))
{
    slideUpAnim->setDuration(300);
    slideDownAnim->setDuration(slideUpAnim->duration());

    connect(slideUpAnim, &QPropertyAnimation::finished, [this]()
    {
        if (!this->dock.isDockVisible)
            this->dock.setDockVisible(false);
        else
        {
            QTimer::singleShot(16, [this](){
                for (int i = 0; i < dockWidget->layout->count(); i++)
                {
                    auto *it = dockWidget->layout->itemAt(i);
                    if (it->widget() && !it->widget()->property("DockItem").isNull())
                    {
                        auto *item = static_cast<DockItem*>(it->widget());
                        item->tooltip->hide();
                        item->tooltip->label->setText(item->controller.name());
                        item->tooltip->show();
                        item->tooltip->windowHandle()->setParent(windowHandle());
                        item->tooltip->move(0, height() - 10);
                        auto *caret = item->tooltip->caret;
                        caret->move(item->tooltip->container->pos() + QPoint((item->tooltip->container->width() - caret->width())/2, item->tooltip->container->size().height() - 2));

                    }
                }
            });
        }
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
