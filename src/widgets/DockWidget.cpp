#include <widgets/DockWidget.h>
#include <widgets/Dock.h>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <controller/Shelf.h>

DockWidget::DockWidget(Dock &dock) noexcept : RoundWidget(&dock, 0.5, 16, QColor(0,0,0,48), QColor(250,250,250,245)),
    dock(dock)
{
    shadowFx->setBlurRadius(64);
    shadowFx->setOffset(0, 8);
    shadowFx->setColor(QColor(0, 0, 0, 64));
    setGraphicsEffect(shadowFx);

    separator->setFixedWidth(2);
    separator->setStyleSheet(".QFrame{background: rgba(0,0,0,0.1); border: 1px solid rgba(0,0,0,0.01); border-radius: 1px;}");
    layout->addWidget(separator, 1);
}

int DockWidget::separatorIndex() const noexcept
{
    return layout->indexOf(separator);
}
