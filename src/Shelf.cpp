#include <LayerShellQt/Shell>
#include <QPropertyAnimation>
#include <QDebug>

#include <Shelf.h>

using namespace LayerShellQt;

Shelf::Shelf() noexcept : QWidget()
{
    setPalette(Qt::transparent);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedHeight(2);

    slideUpAnim = new QPropertyAnimation(dockContainer, "pos", this);
    slideDownAnim = new QPropertyAnimation(dockContainer, "pos", this);
    slideUpAnim->setDuration(200);
    slideDownAnim->setDuration(200);

    connect(slideUpAnim, &QPropertyAnimation::finished,[this]()
    {
        if (!isDockVisible)
            setDockVisible(false);
    });

    connect(slideDownAnim, &QPropertyAnimation::finished,[this]()
    {
        if (isDockVisible)
            setDockVisible(true);
        else
            dockContainer->hide();
    });

    show();

    auto *win = LayerShellQt::Window::get(windowHandle());
    win->setMargins(QMargins(0,0,0,-1));
    win->setExclusiveZone(-1);
    win->setLayer(Window::LayerOverlay);
    win->setAnchors(QFlag(Window::AnchorLeft | Window::AnchorRight | Window::AnchorBottom));

    dockContainer->windowHandle()->setParent(windowHandle());
    dockContainer->hide();
}

void Shelf::setDockVisible(bool visible) noexcept
{
    isDockVisible = visible;

    if (slideUpAnim->state() == QAbstractAnimation::Running || slideDownAnim->state() == QAbstractAnimation::Running)
        return;

    dockContainer->setFixedSize(width(), 96);
    auto *win = LayerShellQt::Window::get(windowHandle());

    if (isDockVisible)
    {
        win->setMargins(QMargins(0,0,0,-4));
        dockContainer->show();
        slideUpAnim->setStartValue(QPoint(0, 2));
        slideUpAnim->setEndValue(QPoint(0, -dockContainer->height()));
        slideUpAnim->start();
    }
    else
    {
        win->setMargins(QMargins(0,0,0,-1));
        dockContainer->show();
        slideDownAnim->setStartValue(QPoint(0, -dockContainer->height()));
        slideDownAnim->setEndValue(QPoint(0, 2));
        slideDownAnim->start();
    }
}

void Shelf::enterEvent(QEnterEvent *event) noexcept
{
    setDockVisible(true);
}


