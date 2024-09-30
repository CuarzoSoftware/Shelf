#include <wayland-client.h>
#include <qpa/qplatformnativeinterface.h>

#include <LayerShellQt/Shell>
#include <QPropertyAnimation>
#include <QLabel>
#include <QDebug>
#include <QtGlobal>
#include <QTimer>

#include <controller/Shelf.h>
#include <controller/ItemController.h>
#include <widgets/Dock.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>

using namespace LayerShellQt;

Dock::Dock(QScreen *screen) noexcept : QWidget()
{
    dockWidget = new DockWidget(*this);
    Shelf::makeWidgetTransparent(this);
    show();hide();
    windowHandle()->setScreen(screen);

    auto &shell { *LayerShellQt::Window::get(windowHandle()) };
    shell.setExclusiveZone(-1);
    shell.setLayer(Window::LayerOverlay);
    shell.setAnchors(QFlag(Window::AnchorLeft | Window::AnchorRight | Window::AnchorBottom));
    shell.setScreenConfiguration(LayerShellQt::Window::ScreenConfiguration::ScreenFromQWindow);

    slideUpAnim = new QPropertyAnimation(this, "offsetY", this);
    slideDownAnim = new QPropertyAnimation(this, "offsetY", this);

    slideUpAnim->setDuration(300);
    slideDownAnim->setDuration(slideUpAnim->duration());

    connect(slideUpAnim, &QPropertyAnimation::finished, [this]()
    {
        if (!isDockVisible)
            setDockVisible(false);
        else
        {
            QTimer::singleShot(16, [this](){
                for (int i = 0; i < dockWidget->layout->count(); i++)
                {
                    auto *it = dockWidget->layout->itemAt(i);
                    if (it->widget() && !it->widget()->property("DockItem").isNull())
                    {
                        auto *item = static_cast<DockItem*>(it->widget());
                        item->tooltip->hide();
                        item->tooltip->label->setText(item->controller.name());
                        item->tooltip->show();
                        item->tooltip->windowHandle()->setParent(windowHandle());
                        item->tooltip->move(2 * i, height() - 10);
                        auto *caret = item->tooltip->caret;
                        caret->move(item->tooltip->container->pos() + QPoint((item->tooltip->container->width() - caret->width())/2, item->tooltip->container->size().height() - 2));

                    }
                }
            });
        }
    });

    connect(slideDownAnim, &QPropertyAnimation::finished,[this]()
    {
        if (isDockVisible)
            setDockVisible(true);
    });

    Shelf::makeWidgetTransparent(dockWidget);
    layout->setContentsMargins(32, 32, 32, 6);
    layout->addStretch(1);
    layout->addWidget(dockWidget);
    layout->addStretch(1);
    updateDimensions();
    show();
    qInfo() << "Dock added to screen" << screen->name();
}

Dock::~Dock()
{
    qInfo() << "Dock removed from screen" << screen()->name();
}

void Dock::setOffsetY(int offsetY)
{
    auto &shell { *LayerShellQt::Window::get(windowHandle()) };
    if (offsetY != shell.margins().bottom())
    {
        shell.setMargins(QMargins(0, 0, 0, offsetY));

        // Ugly fix to prevent calling update() which damages entire buffer
        wl_surface *surface = (wl_surface*)Shelf::get().platformNativeInterface()->nativeResourceForWindow("surface", windowHandle());
        if (surface)
            wl_surface_commit(surface);

        offsetYChanged();
    }
}

int Dock::offsetY() const
{
    auto &shell { *LayerShellQt::Window::get(windowHandle()) };
    return shell.margins().bottom();
}

void Dock::setDockVisible(bool visible) noexcept
{
    isDockVisible = visible;

    if (slideUpAnim->state() == QAbstractAnimation::Running || slideDownAnim->state() == QAbstractAnimation::Running)
        return;

    auto &shell { *LayerShellQt::Window::get(windowHandle()) };

    if (isDockVisible)
    {
        slideUpAnim->setStartValue(2 - height());
        slideUpAnim->setEndValue(-2);
        slideUpAnim->start();
    }
    else
    {
        for (int i = 0; i < dockWidget->layout->count(); i++)
        {
            auto *it = dockWidget->layout->itemAt(i);
            if (it->widget() && !it->widget()->property("DockItem").isNull())
                static_cast<DockItem*>(it->widget())->tooltip->hide();
        }

        slideDownAnim->setStartValue(-2);
        slideDownAnim->setEndValue(2 - height());
        slideDownAnim->start();
    }
}

void Dock::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDimensions();

    if (!isDockVisible)
        setOffsetY(2 - height());
}

void Dock::updateDimensions() noexcept
{
    itemSize.setWidth(maxItemSize());
    dotSize = itemSize.width() * 0.05;

    if (dotSize < 2)
        dotSize = 2;

    if (dotSize > 5)
        dotSize = 5;

    dotMarginTop = itemSize.width() * 0.12;
    dotMarginBottom = itemSize.width() * 0.025;

    if (dotMarginBottom < 2)
        dotMarginBottom = 2;

    tooltipDistY = itemSize.width() * 0.2;
    itemSize.setHeight(itemSize.width() + dotMarginTop + dotSize + dotMarginBottom);
    itemsSpacing = itemSize.width() * 0.18;
    dockBorderRadius = itemSize.width() * 0.38;
    dockMargins.setTop(itemSize.width() * 0.18);
    dockMargins.setLeft(dockMargins.top());
    dockMargins.setRight(dockMargins.top());
    dockMargins.setBottom(0);
    dockSize.setHeight(dockMargins.top() + dockMargins.bottom() + itemSize.height());

    dockWidget->layout->setContentsMargins(dockMargins);
    dockWidget->layout->setSpacing(itemsSpacing);
    dockWidget->setFixedHeight(dockSize.height());
    dockWidget->setBorderRadius(dockBorderRadius);
    setFixedSize(width(), dockSize.height() + layout->contentsMargins().top() + layout->contentsMargins().bottom() + 2);

    //dockElevator->windowHandle()->setMask(QRegion(0, 32, dockElevator->width(), dockElevator->height()));

    for (int i = 0; i < dockWidget->layout->count(); i++)
    {
        auto *it = dockWidget->layout->itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
        {
            static_cast<DockItem*>(it->widget())->updateDimensions();
        }
    }

    dockWidget->separator->setFixedHeight(dockWidget->height() - 2 * dockMargins.top());
}

int Dock::maxItemSize() const noexcept
{
    double N = dockWidget->layout->count() - 1;
    int maxWidth = double(width()) / (N + 0.18*N + 3.18);
    int desired = dockScale * 62;

    return qMin(maxWidth, desired);
}

void Dock::enterEvent(QEnterEvent *event) noexcept
{
    isMouseOver = true;
    setDockVisible(true);
}

void Dock::leaveEvent(QEvent *) noexcept
{
    isMouseOver = false;
    setDockVisible(false);
}


