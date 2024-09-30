#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <widgets/RoundWidget.h>
#include <widgets/ItemSeparator.h>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

class Dock;
class DockElevator;

class DockWidget final : public RoundWidget
{
public:
    DockWidget(Dock &dock) noexcept;
    int separatorIndex() const noexcept;

    void resizeEvent(QResizeEvent *) override;

    Dock &dock;
    QGraphicsDropShadowEffect *shadowFx { new QGraphicsDropShadowEffect(this) };
    QHBoxLayout *layout { new QHBoxLayout(this) };
    ItemSeparator *separator;
};

#endif // DOCKWIDGET_H
