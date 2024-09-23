#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

class Dock;
class DockElevator;

class DockWidget final : public QFrame
{
public:
    DockWidget(Dock &dock) noexcept;
    Dock &dock;
    QGraphicsDropShadowEffect *shadowFx { new QGraphicsDropShadowEffect(this) };
    QHBoxLayout *layout { new QHBoxLayout(this) };
};

#endif // DOCKWIDGET_H
