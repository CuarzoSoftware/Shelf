#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <widgets/RoundWidget.h>
#include <widgets/Caret.h>
#include <controller/Constants.h>

class DockItem;

class Tooltip final : public QWidget
{
public:
    Tooltip(QWidget *parent = nullptr) noexcept;
    QVBoxLayout *layout { new QVBoxLayout(this) };

    RoundWidget *container { new RoundWidget(this, 0.5, 5.0, QColor(0, 0, 0, 48), DOCK_COLOR) };
    QVBoxLayout *containerLayout { new QVBoxLayout(container) };
    QLabel *label { new QLabel("Unknown", this) };
    Caret *caret { new Caret(this) };

    QGraphicsDropShadowEffect *shadow { new  QGraphicsDropShadowEffect(this) };

    void showName(DockItem *item) noexcept;
};

#endif // TOOLTIP_H
