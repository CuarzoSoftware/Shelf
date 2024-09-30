#include <widgets/Caret.h>
#include <QPainter>
#include <QPainterPath>

Caret::Caret(QWidget *parent) noexcept : QWidget(parent)
{
    setFixedSize(18, 10);
}

void Caret::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    QPen pen;
    pen.setColor(QColor(0, 0, 0, 48));
    pen.setWidthF(0.5);
    painter.setPen(pen);
    painter.setBrush(QColor(250,250,250,245));

    const double yOffset { 1.0 };

    QPainterPath path;
    path.moveTo(0, -5);
    path.lineTo(0, yOffset);

    path.cubicTo(
        3, yOffset,
        width()/2 - 2, height(),
        width()/2, height());

    path.cubicTo(
        width()/2 + 2, height(),
        width() - 3, 0,
        width(), yOffset);

    path.lineTo(width(), -5);
    painter.fillPath(path, painter.brush());
    painter.drawPath(path);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0,0, width(), 1);
}
