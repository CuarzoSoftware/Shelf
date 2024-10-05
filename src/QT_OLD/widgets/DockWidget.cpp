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

    dock.items.push_back(&separator);
    separatorDockLink = std::prev(dock.items.end());
}

DockWidget::~DockWidget()
{
    dock.items.erase(separatorDockLink);
}

void DockWidget::resizeEvent(QResizeEvent *)
{
    for (QWidget *it : dock.items)
    {
        if (!it->property("DockItem").isNull())
        {
            if (static_cast<DockItem*>(it)->mouseIsOver)
                static_cast<DockItem*>(it)->tooltip.showItemName(static_cast<DockItem*>(it));
        }
    }
}
