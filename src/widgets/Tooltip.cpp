#include <controller/Shelf.h>
#include <controller/ItemController.h>
#include <widgets/Tooltip.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QWindow>
#include <QTimer>

Tooltip::Tooltip(QWidget *parent) noexcept : QWidget(parent)
{
    static constexpr int margin { 16 };
    layout->setContentsMargins(margin, margin, margin, margin);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(0, 0, 0, 64));
    setGraphicsEffect(shadow);
    Shelf::makeWidgetTransparent(this);
    layout->addWidget(container);
    show(); hide();
    layout->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(".QLabel{color:#1a1a1a;font-family:Inter}");
    containerLayout->addWidget(label);
}

void Tooltip::showName(DockItem *item) noexcept
{
    if (item->dock.slideDownAnim->state() == QPropertyAnimation::Running || item->dock.slideUpAnim->state() == QPropertyAnimation::Running)
        return;

    if (label->text() != item->controller.name())
        label->setText(item->controller.name());

    const QPoint itemPos { item->mapTo(&item->dock, QPoint(0, 0)) };
    windowHandle()->setParent(item->dock.windowHandle());
    show();

    for (int i = 0; i < item->dock.dockWidget->layout->count(); i++)
    {
        auto *it = item->dock.dockWidget->layout->itemAt(i);
        if (it->widget() && it->widget() != item && !it->widget()->property("DockItem").isNull())
            static_cast<DockItem*>(it->widget())->tooltip->move(0, 1000);
    }

    move(itemPos.x() + (item->width() - width())/2, itemPos.y() -height() - item->dock.tooltipDistY);
    caret->move(container->pos() + QPoint((container->width() - caret->width())/2, container->size().height() - 2));
}
