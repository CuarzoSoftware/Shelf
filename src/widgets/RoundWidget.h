#ifndef ROUNDWIDGET_H
#define ROUNDWIDGET_H

#include <QWidget>
#include <QPen>

class RoundWidget : public QWidget
{
public:
    RoundWidget(QWidget *parent, double borderSize, double borderRadius, const QColor &borderColor, const QColor &backgroundColor) noexcept;

    void setBorderRadius(double borderRadius) noexcept
    {
        m_borderRadius = borderRadius;
        update();
    }
private:
    QPen m_pen;
    double m_borderRadius;
    QColor m_backgroundColor;
    void paintEvent(QPaintEvent *) override;
};

#endif // ROUNDWIDGET_H
