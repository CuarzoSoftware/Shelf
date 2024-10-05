#ifndef CARET_H
#define CARET_H

#include <QWidget>

class Caret final : public QWidget
{
public:
    Caret(QWidget *parent) noexcept;

    void paintEvent(QPaintEvent *) override;
};

#endif // CARET_H
