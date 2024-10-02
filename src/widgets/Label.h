#ifndef LABEL_H
#define LABEL_H

#include <QLabel>

class Label : public QLabel
{
    Q_OBJECT
public:
    Label(QWidget *parent) noexcept;

    void showEvent(QShowEvent *event) override
    {
        visibilityChanged(true);
    }

    void hideEvent(QHideEvent *event) override
    {
        visibilityChanged(false);
    }

Q_SIGNALS:
    void visibilityChanged(bool visible);
};

#endif // LABEL_H
