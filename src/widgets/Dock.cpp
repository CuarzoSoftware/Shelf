#include <LayerShellQt/Shell>
#include <QPropertyAnimation>
#include <QLabel>
#include <QDebug>

#include <controller/Shelf.h>
#include <widgets/Dock.h>
#include <widgets/DockElevator.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>

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
    updateDimensions();

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

void Dock::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDimensions();
}

void Dock::updateDimensions() noexcept
{
    itemSize.setWidth(dockScale * 62);
    itemSize.setHeight(itemSize.width());
    itemsSpacing = itemSize.width() * 0.18;
    dockBorderRadius = itemSize.width() * 0.38;
    dockMargins.setTop(itemSize.width() * 0.18);
    dockMargins.setLeft(dockMargins.top());
    dockMargins.setRight(dockMargins.top());
    dockMargins.setBottom(itemSize.width() * 0.26);
    dockSize.setHeight(dockMargins.top() + dockMargins.bottom() + itemSize.height());

    dockElevator->dockWidget->layout->setContentsMargins(dockMargins);
    dockElevator->dockWidget->layout->setSpacing(itemsSpacing);
    dockElevator->dockWidget->setFixedHeight(dockSize.height());
    dockElevator->dockWidget->setStyleSheet(
        ".QFrame{background: qlineargradient(spread:pad, x1:0, y1:0.5, x2:1, y2:0.5, stop:0 #3c27ac, stop:1 #5d1eff); border: 1px solid rgba(0,0,0,0.1); border-radius: " + QString::number(dockBorderRadius) + "px;}");
    dockElevator->setFixedSize(width(), dockSize.height() + 42);

    dockElevator->windowHandle()->setMask(QRegion(0, 32, dockElevator->width(), dockElevator->height()));

    for (int i = 0; i < dockElevator->dockWidget->layout->count(); i++)
    {
        auto *it = dockElevator->dockWidget->layout->itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
            static_cast<DockItem*>(it->widget())->updateDimensions();
    }
}

void Dock::enterEvent(QEnterEvent *event) noexcept
{
    setDockVisible(true);
}


