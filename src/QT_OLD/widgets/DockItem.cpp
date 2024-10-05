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

    setParent(&dock.dockWidget);

    switch (controller.type())
    {
    case ItemController::App:
        dockLink = dock.items.insert(dock.dockWidget.separatorDockLink, this);
        break;
    case ItemController::Thumbnail:
        dockLink = dock.items.insert(std::prev(dock.items.end()), this);
        break;
    case ItemController::Trash:
        dock.items.push_back(this);
        dockLink = std::prev(dock.items.end());
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
    dock.items.erase(dockLink);
    controller.items().erase(controllerLink);
    dock.updateDimensions();
}

void DockItem::updateDimensions() noexcept
{
    setFixedSize(dock.itemSize.width() * controller.itemScale(), dock.itemSize.height());
    dot.setFixedSize(dock.dotSize, dock.dotSize);
    layout.setContentsMargins(0, 0, 0, dock.dotMarginBottom);
    layout.setSpacing(dock.dotMarginTop);
    icon.setFixedSize(width(), width());
    iconSubstitute.setFixedSize(icon.size());
    jumpingIcon.setFixedSize(icon.size());

    if (controller.itemScale() == 1.0)
    {
        layout.insertWidget(0, &icon, 0, Qt::AlignCenter);
    }
    else
    {
        layout.removeWidget(&icon);
        QSizeF s {(size().toSizeF() - icon.size().toSizeF())/2.f};
        icon.move(s.width(), s.height());
        show();
        icon.show();
    }
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
