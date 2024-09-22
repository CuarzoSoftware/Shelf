#ifndef DOCK_H
#define DOCK_H

#include <QFrame>
#include <QGraphicsDropShadowEffect>

class Shelf;

class Dock final : public QFrame
{
public:
    Dock(Shelf *shelf);
    Shelf *shelf;
    QGraphicsDropShadowEffect *shadowFx { new QGraphicsDropShadowEffect(this) };
};

#endif // DOCK_H
