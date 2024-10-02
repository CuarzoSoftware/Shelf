#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <controller/Shelf.h>
#include <controller/Constants.h>

DockWidget::DockWidget(Dock &dock) noexcept : RoundWidget(&dock, 0.5, 16, QColor(0,0,0,48), DOCK_COLOR),
    dock(dock),
    separator(this, dock)
{
    Shelf::makeWidgetTransparent(&dock);
    dock.show();dock.hide();
    shadowFx.setBlurRadius(96);
    shadowFx.setOffset(0, 10);
    shadowFx.setColor(QColor(0, 0, 0, 96));
    setGraphicsEffect(&shadowFx);

    layout.addWidget(&separator, 1, Qt::AlignTop);
    layout.setContentsMargins(0, 0, 0, 0);
}

int DockWidget::separatorIndex() const noexcept
{
    return layout.indexOf(&separator);
}

void DockWidget::resizeEvent(QResizeEvent *)
{
    for (int i = 0; i < layout.count(); i++)
    {
        auto *it = layout.itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
        {
            if (static_cast<DockItem*>(it->widget())->mouseIsOver)
                static_cast<DockItem*>(it->widget())->tooltip.showItemName(static_cast<DockItem*>(it->widget()));
        }
    }
}
