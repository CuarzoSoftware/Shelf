#ifndef ITEMSEPARATOR_H
#define ITEMSEPARATOR_H

#include <QFrame>
#include <QHBoxLayout>

class Dock;

class ItemSeparator final : public QWidget
{
public:
    ItemSeparator(QWidget *parent, Dock &dock) noexcept;
    Dock &dock;

    bool dragging { false };
    double initCursorX;
    double initDockScale;

    QVBoxLayout layout { this };
    QFrame line { this };

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // ITEMSEPARATOR_H
