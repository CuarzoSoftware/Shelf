#include <widgets/ItemSeparator.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <controller/Shelf.h>
#include <QMouseEvent>
#include <QCursor>
#include <wayland-client.h>
#include <qpa/qplatformnativeinterface.h>

ItemSeparator::ItemSeparator(QWidget *parent, Dock &dock) noexcept : QWidget(parent), dock(dock)
{
    Shelf::makeWidgetTransparent(this);
    setFixedWidth(6);
    line.setStyleSheet(".QFrame{background: rgba(0,0,0,0.1); border: 1px solid rgba(0,0,0,0.01); border-radius: 1px;}");
    line.setFixedWidth(2);
    layout.setSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.addWidget(&line, 1, Qt::AlignHCenter);
    setCursor(QCursor(Qt::SizeVerCursor));
    setMouseTracking(true);
}

void ItemSeparator::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragging = true;
        initCursorX = QCursor::pos().x();
        initDockScale = dock.dockScale;

        for (QWidget *it : dock.items)
            if (!it->property("DockItem").isNull())
                static_cast<DockItem*>(it)->tooltip.move(0, 1000);
    }
}

void ItemSeparator::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragging = false;
        dock.updateDimensions();
    }
}

void ItemSeparator::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging)
    {
        double delta = (double(QCursor::pos().x()) - initCursorX) * 0.005;
        dock.dockScale = initDockScale + delta;

        if (dock.dockScale > 1.0)
            dock.dockScale = 1.0;

        if (dock.dockScale < 0.25)
            dock.dockScale = 0.25;

        dock.updateDimensions();
    }
}
