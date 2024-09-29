#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <widgets/RoundWidget.h>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QFrame>

class Dock;
class DockElevator;

class DockWidget final : public RoundWidget
{
public:
    DockWidget(Dock &dock) noexcept;
    int separatorIndex() const noexcept;

    Dock &dock;
    QGraphicsDropShadowEffect *shadowFx { new QGraphicsDropShadowEffect(this) };
    QHBoxLayout *layout { new QHBoxLayout(this) };
    QFrame *separator { new QFrame(this) };
};

#endif // DOCKWIDGET_H
