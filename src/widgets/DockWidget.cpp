#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <controller/Shelf.h>

DockWidget::DockWidget(Dock &dock) noexcept : RoundWidget(&dock, 0.5, 16, QColor(0,0,0,48), QColor(250,250,250,245)),
    dock(dock),
    separator(new ItemSeparator(this, dock))
{
    shadowFx->setBlurRadius(64);
    shadowFx->setOffset(0, 8);
    shadowFx->setColor(QColor(0, 0, 0, 64));
    setGraphicsEffect(shadowFx);

    layout->addWidget(separator, 1, Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
}

int DockWidget::separatorIndex() const noexcept
{
    return layout->indexOf(separator);
}

void DockWidget::resizeEvent(QResizeEvent *)
{
    for (int i = 0; i < layout->count(); i++)
    {
        auto *it = layout->itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
        {
            if (static_cast<DockItem*>(it->widget())->mouseIsOver)
                static_cast<DockItem*>(it->widget())->tooltip->showName(static_cast<DockItem*>(it->widget()));
        }
    }
}
