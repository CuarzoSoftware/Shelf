#include <widgets/RoundWidget.h>
#include <QPainter>

RoundWidget::RoundWidget(QWidget *parent, double borderSize, double borderRadius, const QColor &borderColor, const QColor &backgroundColor) noexcept :
    QWidget(parent),
    m_pen(borderColor),
    m_borderRadius(borderRadius),
    m_backgroundColor(backgroundColor)
{
    m_pen.setWidthF(borderSize);
}

void RoundWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(m_pen);
    painter.setBrush(m_backgroundColor);
    const double border { m_pen.widthF() * 2.0 };
    painter.drawRoundedRect(QRect(border, border, width() - border * 2, height() - border * 2), m_borderRadius, m_borderRadius);
}
