#include <controller/ItemController.h>
#include <controller/Shelf.h>
#include <widgets/Tooltip.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>
#include <QDebug>

DockItem::DockItem(ItemController &controller, Dock &dock) noexcept :
    QWidget(),
    dock(dock),
    controller(controller)
{
    Shelf::makeWidgetTransparent(&jumpingIcon);
    jumpingIcon.setParent(nullptr);
    jumpingIcon.show();
    jumpingIcon.hide();
    jumpingIcon.windowHandle()->setParent(dock.windowHandle());
    jumpingIcon.windowHandle()->setFlag(Qt::WindowTransparentForInput);
    jumpingIcon.setScaledContents(true);
    jumpingIcon.setPixmap(controller.icon());

    darkFx.setEnabled(false);
    darkFx.setColor(Qt::black);
    setGraphicsEffect(&darkFx);

    setProperty("DockItem", true);
    controller.items().push_back(this);
    controllerLink = std::prev(controller.items().end());

    icon.setScaledContents(true);
    icon.setPixmap(controller.icon());
    icon.setFixedSize(36, 36);

    dot.setPixmap(Shelf::get().dot);
    dot.setScaledContents(true);
    dot.setVisible(controller.running());
    layout.addWidget(&iconSubstitute, 0, Qt::AlignCenter);
    iconSubstitute.hide();

    layout.addWidget(&icon, 0, Qt::AlignCenter);
    layout.addWidget(&dot, 0, Qt::AlignCenter);
    layout.addStretch(1);

    switch (controller.type())
    {
    case ItemController::App:
        dock.dockWidget.layout.insertWidget(
            dock.dockWidget.separatorIndex(),
            this);
        break;
    case ItemController::Trash:
        dock.dockWidget.layout.addWidget(this);
        break;
    }

    connect(&jumpingIcon, &Label::visibilityChanged, [this](bool visible) {
        iconSubstitute.setVisible(visible);
        icon.setVisible(!visible);
    });

    dock.updateDimensions();
}

DockItem::~DockItem() noexcept
{
    controller.items().erase(controllerLink);
}

void DockItem::updateDimensions() noexcept
{
    setFixedSize(dock.itemSize);
    dot.setFixedSize(dock.dotSize, dock.dotSize);
    layout.setContentsMargins(0, 0, 0, dock.dotMarginBottom);
    layout.setSpacing(dock.dotMarginTop);

    if (controller.icon().width() <= 0 || controller.icon().height() <= 0)
        return;

    if (controller.icon().width() > controller.icon().height())
    {
        icon.setFixedWidth(width());
        icon.setFixedHeight((controller.icon().height() * width())/controller.icon().width());
    }
    else
    {
        icon.setFixedHeight(width());
        icon.setFixedWidth((controller.icon().width() * width())/controller.icon().height());
    }

    iconSubstitute.setFixedSize(icon.size());
    jumpingIcon.setFixedSize(icon.size());
}

void DockItem::enterEvent(QEnterEvent *)
{
    mouseIsOver = true;
    tooltip.showItemName(this);
}

void DockItem::leaveEvent(QEvent *)
{
    mouseIsOver = false;
}

void DockItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        icon.setPixmap(controller.iconDark());
}

void DockItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        controller.onItemClick(this);
        icon.setPixmap(controller.icon());
    }
}
