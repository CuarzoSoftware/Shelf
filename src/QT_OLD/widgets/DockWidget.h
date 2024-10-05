#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <widgets/RoundWidget.h>
#include <widgets/ItemSeparator.h>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

class Dock;
class DockElevator;

/* Dock items container */

class DockWidget final : public RoundWidget
{
public:
    DockWidget(Dock &dock) noexcept;
    ~DockWidget();
    void resizeEvent(QResizeEvent *) override;

    Dock &dock;
    QGraphicsDropShadowEffect shadowFx { this };
    ItemSeparator separator;
    std::list<QWidget*>::iterator separatorDockLink;
};

#endif // DOCKWIDGET_H
