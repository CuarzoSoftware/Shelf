#include <controller/ItemController.h>
#include <controller/Shelf.h>
#include <widgets/Tooltip.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

DockItem::DockItem(ItemController &controller, Dock &dock) noexcept :
    QWidget(),
    dock(dock),
    controller(controller)
{
    darkFx->setColor(Qt::black);
    setGraphicsEffect(darkFx);
    darkFx->setEnabled(false);

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

    dot->setPixmap(Shelf::get().dot);
    dot->setScaledContents(true);
    dot->setVisible(controller.active());
    layout->addWidget(icon, 0, Qt::AlignCenter);
    layout->addWidget(dot, 0, Qt::AlignCenter);
    layout->addStretch(1);

    switch (controller.type())
    {
    case ItemController::App:
        dock.dockWidget->layout->insertWidget(
            dock.dockWidget->separatorIndex(),
            this);
        break;
    case ItemController::Trash:
        dock.dockWidget->layout->addWidget(this);
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
    dot->setFixedSize(dock.dotSize, dock.dotSize);
    layout->setContentsMargins(0, 0, 0, dock.dotMarginBottom);
    layout->setSpacing(dock.dotMarginTop);

    if (controller.icon().width() <= 0 || controller.icon().height() <= 0)
        return;

    if (controller.icon().width() > controller.icon().height())
    {
        icon->setFixedWidth(width());
        icon->setFixedHeight((controller.icon().height() * width())/controller.icon().width());
    }
    else
    {
        icon->setFixedHeight(width());
        icon->setFixedWidth((controller.icon().width() * width())/controller.icon().height());
    }
}

void DockItem::enterEvent(QEnterEvent *)
{
    mouseIsOver = true;
    tooltip->showName(this);
}

void DockItem::leaveEvent(QEvent *)
{
    mouseIsOver = false;
}

void DockItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPixmap dark = controller.icon();
        QPainter p(&dark);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 64));
        p.drawRect(0,0,dark.width(),dark.height());
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.drawPixmap(0, 0, controller.icon());
        p.end();
        icon->setPixmap(dark);
    }
}

void DockItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        controller.onItemClick(this);
        icon->setPixmap(controller.icon());
    }
}
