#include <controller/Shelf.h>
#include <controller/ItemController.h>
#include <widgets/Tooltip.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QWindow>
#include <QTimer>
#include <QScreen>

Tooltip::Tooltip(QWidget *parent) noexcept : QWidget(parent)
{
    static constexpr int margin { 16 };
    layout.setContentsMargins(margin, margin, margin, margin);
    shadow.setBlurRadius(48);
    shadow.setOffset(0, 1);
    shadow.setColor(QColor(0, 0, 0, 64));
    setGraphicsEffect(&shadow);
    Shelf::makeWidgetTransparent(this);
    layout.addWidget(&container);
    show(); hide();
    layout.setAlignment(Qt::AlignCenter);
    label.setStyleSheet(".QLabel{color:#1a1a1a;font-family:Inter}");
    containerLayout.addWidget(&label);
    windowHandle()->setFlag(Qt::WindowTransparentForInput);
}

void Tooltip::showItemName(DockItem *item) noexcept
{
    if (item->dock.slideDownAnim.state() == QPropertyAnimation::Running
        || item->dock.slideUpAnim.state() == QPropertyAnimation::Running)
        return;

    if (label.text() != item->controller.name())
        label.setText(item->controller.name());

    shadow.setBlurRadius(24 * item->dock.screen()->devicePixelRatio());

    const QPoint itemPos { item->mapTo(&item->dock, QPoint(0, 0)) };
    windowHandle()->setParent(item->dock.windowHandle());

    for (int i = 0; i < item->dock.dockWidget.layout.count(); i++)
    {
        auto *it = item->dock.dockWidget.layout.itemAt(i);
        if (it->widget() && it->widget() != item && !it->widget()->property("DockItem").isNull())
            static_cast<DockItem*>(it->widget())->tooltip.move(0, 1000);
    }

    QPoint newPos { itemPos.x() + (item->width() - width())/2, itemPos.y() -height() - item->dock.tooltipDistY };
    QPoint caretPos { container.pos() + QPoint((container.width() - caret.width())/2, container.size().height() - 2) };

    if (newPos.x() < 0)
    {
        caretPos.setX(caretPos.x() + newPos.x());
        newPos.setX(0);
        update();
    }
    else if (newPos.x() + width() > item->dock.width())
    {
        caretPos.setX(caretPos.x() - item->dock.width() + newPos.x() + width());
        newPos.setX(item->dock.width() - width());
        update();
    }

    caret.move(caretPos);
    move(newPos);
    show();
}
