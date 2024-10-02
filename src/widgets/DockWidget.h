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
    int separatorIndex() const noexcept;
    void resizeEvent(QResizeEvent *) override;

    Dock &dock;
    QHBoxLayout layout { this };
    QGraphicsDropShadowEffect shadowFx { this };
    ItemSeparator separator;
};

#endif // DOCKWIDGET_H
