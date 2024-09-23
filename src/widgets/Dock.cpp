#include <LayerShellQt/Shell>
#include <QPropertyAnimation>
#include <QDebug>

#include <controller/Shelf.h>
#include <widgets/Dock.h>
#include <widgets/DockElevator.h>

using namespace LayerShellQt;

Dock::Dock(QScreen *screen) noexcept : QWidget(),
    dockElevator(new DockElevator(*this))
{
    Shelf::makeWidgetTransparent(this);
    setFixedHeight(2);
    show();hide();
    windowHandle()->setScreen(screen);

    auto &shell { *LayerShellQt::Window::get(windowHandle()) };
    shell.setMargins(QMargins(0,0,0,-1));
    shell.setExclusiveZone(-1);
    shell.setLayer(Window::LayerOverlay);
    shell.setAnchors(QFlag(Window::AnchorLeft | Window::AnchorRight | Window::AnchorBottom));
    shell.setScreenConfiguration(LayerShellQt::Window::ScreenConfiguration::ScreenFromQWindow);
    show();

    dockElevator->windowHandle()->setParent(windowHandle());
    dockElevator->hide();

    qInfo() << "Dock added to screen" << screen->name();
}

Dock::~Dock()
{
    qInfo() << "Dock removed from screen" << screen()->name();
}

void Dock::setDockVisible(bool visible) noexcept
{
    isDockVisible = visible;

    if (dockElevator->slideUpAnim->state() == QAbstractAnimation::Running ||
        dockElevator->slideDownAnim->state() == QAbstractAnimation::Running)
        return;

    dockElevator->setFixedSize(width(), 96);

    auto &shell { *LayerShellQt::Window::get(windowHandle()) };

    if (isDockVisible)
    {
        shell.setMargins(QMargins(0,0,0,-4));
        dockElevator->show();
        dockElevator->slideUpAnim->setStartValue(QPoint(0, 2));
        dockElevator->slideUpAnim->setEndValue(QPoint(0, -dockElevator->height()));
        dockElevator->slideUpAnim->start();
    }
    else
    {
        shell.setMargins(QMargins(0,0,0,-1));
        dockElevator->show();
        dockElevator->slideDownAnim->setStartValue(QPoint(0, -dockElevator->height()));
        dockElevator->slideDownAnim->setEndValue(QPoint(0, 2));
        dockElevator->slideDownAnim->start();
    }
}

void Dock::enterEvent(QEnterEvent *event) noexcept
{
    setDockVisible(true);
}


