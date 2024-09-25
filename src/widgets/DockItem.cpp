#include <controller/ItemController.h>
#include <widgets/DockWidget.h>
#include <widgets/DockElevator.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>

DockItem::DockItem(ItemController &controller, Dock &dock) noexcept :
    QWidget(),
    dock(dock),
    controller(controller)
{
    setProperty("DockItem", true);
    controller.items().push_back(this);
    controllerLink = std::prev(controller.items().end());

    shadow->setBlurRadius(4);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(0, 0, 0, 64));
    icon->setGraphicsEffect(shadow);
    icon->setScaledContents(true);
    icon->setPixmap(controller.icon());
    icon->setFixedSize(36, 36);

    switch (controller.type())
    {
    case ItemController::App:
        dock.dockElevator->dockWidget->layout->insertWidget(
            dock.dockElevator->dockWidget->separatorIndex(),
            this);
        break;
    case ItemController::Trash:
        dock.dockElevator->dockWidget->layout->addWidget(this);
        break;
    }

    dock.updateDimensions();
}

DockItem::~DockItem() noexcept
{
    controller.items().erase(controllerLink);
}

void DockItem::updateDimensions() noexcept
{
    setFixedSize(dock.itemSize);

    if (controller.icon().width() <= 0 || controller.icon().height() <= 0)
    {
        qInfo() << "Invalid size";
        return;
    }

    if (controller.icon().width() > controller.icon().height())
    {
        icon->setFixedWidth(width());
        icon->setFixedHeight((controller.icon().height() * width())/controller.icon().width());
        icon->move(0, (height() - icon->height())/2);
    }
    else
    {
        icon->setFixedHeight(height());
        icon->setFixedWidth((controller.icon().width() * height())/controller.icon().height());
        icon->move((width() - icon->width())/2, 0);
    }
}
