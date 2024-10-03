#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <widgets/RoundWidget.h>
#include <widgets/Caret.h>
#include <controller/Constants.h>
#include <QOpenGLWidget>

class DockItem;

class Tooltip final : public QWidget
{
public:
    Tooltip(QWidget *parent = nullptr) noexcept;
    QVBoxLayout layout { this };
    RoundWidget container { this, 0.5, 5.0, QColor(0, 0, 0, 48), DOCK_COLOR };
    QVBoxLayout containerLayout { &container };
    QLabel label { "Unknown", this };
    Caret caret { this };
    QGraphicsDropShadowEffect shadow { this };

    void showItemName(DockItem *item) noexcept;
};

#endif // TOOLTIP_H
