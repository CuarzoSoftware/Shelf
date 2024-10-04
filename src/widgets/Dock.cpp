#include <wayland-client.h>
#include <qpa/qplatformnativeinterface.h>

#include <LayerShellQt/Shell>
#include <QPropertyAnimation>
#include <QLabel>
#include <QDebug>
#include <QtGlobal>
#include <QTimer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>

#include <controller/Shelf.h>
#include <controller/ItemController.h>
#include <controller/AppController.h>
#include <widgets/Dock.h>
#include <widgets/DockWidget.h>
#include <widgets/DockItem.h>

using namespace LayerShellQt;

Dock::Dock(QScreen *screen) noexcept : QWidget()
{
    windowHandle()->setScreen(screen);
    auto &shell { *LayerShellQt::Window::get(windowHandle()) };
    shell.setExclusiveZone(-1);
    shell.setLayer(Window::LayerOverlay);
    shell.setAnchors(QFlag(Window::AnchorLeft | Window::AnchorRight | Window::AnchorBottom));
    shell.setScreenConfiguration(LayerShellQt::Window::ScreenConfiguration::ScreenFromQWindow);
    shell.setScope("cuarzo-shelf");

    slideUpAnim.setDuration(DOCK_TOGGLE_MS);
    slideDownAnim.setDuration(slideUpAnim.duration());

    connect(&slideUpAnim, &QPropertyAnimation::finished, [this]()
    {
        windowHandle()->setMask(QRegion(0, dockWidget.y(), width(), height()));

        if (!isDockVisible)
            setDockVisible(false);
        else
        {
            QTimer::singleShot(16, [this](){
                int i { 0 };
                for (QWidget *it : items)
                {
                    if (!it->property("DockItem").isNull())
                    {
                        auto *item = static_cast<DockItem*>(it);
                        item->tooltip.hide();
                        item->tooltip.label.setText(item->controller.name());
                        item->tooltip.show();
                        item->tooltip.windowHandle()->setParent(windowHandle());
                        item->tooltip.move(2 * i, height() - 10);
                        auto &caret = item->tooltip.caret;
                        caret.move(
                            item->tooltip.container.pos() + QPoint((item->tooltip.container.width() - caret.width())/2,
                            item->tooltip.container.size().height() - 2));
                        item->tooltip.update();
                    }

                    i++;
                }
            });
        }
    });

    connect(&slideDownAnim, &QPropertyAnimation::finished, [this]()
    {
        windowHandle()->setMask(QRegion(0, 0, width(), height()));
        setOffsetY(4 - height());

        if (isDockVisible)
            setDockVisible(true);
    });

    for (AppController *app : Shelf::get().pinnedApps)
        new DockItem(*app, *this);

    new DockItem((AppController&)*Shelf::get().trash, *this);

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

        //update(0, 0, 1, 1);

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

    if (slideUpAnim.state() == QAbstractAnimation::Running || slideDownAnim.state() == QAbstractAnimation::Running)
        return;

    auto &shell { *LayerShellQt::Window::get(windowHandle()) };

    if (isDockVisible)
    {
        slideUpAnim.setStartValue(4 -dockAndMargins);
        slideUpAnim.setEndValue(-4);
        slideUpAnim.start();
    }
    else
    {
        for (QWidget *it : items)
        {
            if (!it->property("DockItem").isNull())
                static_cast<DockItem*>(it)->tooltip.hide();
        }

        slideDownAnim.setStartValue(-4);
        slideDownAnim.setEndValue(4-dockAndMargins);
        slideDownAnim.start();
    }
}

void Dock::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    maxHeight = sqrt(pow(screen()->size().width(), 2.0) + pow(screen()->size().height(), 2.0)) * DOCK_MAX_HEIGHT;
    setFixedSize(width(), maxHeight);
    updateDimensions();
    repaint();

    if (!isDockVisible)
        setOffsetY(2 - height());
    else
        hideAllTooltips();
}

void Dock::updateDimensions() noexcept
{
    assert(screen() != nullptr);

    itemSize.setWidth(maxItemSize());
    dotSize = itemSize.width() * 0.08;

    double shadowM = double(screen()->devicePixelRatio() * itemSize.width()) * 0.125;
    dockWidget.shadowFx.setBlurRadius(shadowM * 2.0);
    dockWidget.shadowFx.setOffset(0, shadowM * 0.2);
    dockAndMargins = itemSize.height() + dockWidget.shadowFx.blurRadius() + DOCK_MARGIN_BOTTOM;

    if (dotSize < 3)
        dotSize = 3;

    if (dotSize > 5)
        dotSize = 5;

    dotMarginTop = itemSize.width() * 0.10;
    dotMarginBottom = itemSize.width() * 0.025;

    if (dotMarginBottom < 2)
        dotMarginBottom = 2;

    tooltipDistY = itemSize.width() * 0.2;
    itemSize.setHeight(itemSize.width() + dotMarginTop + dotSize + dotMarginBottom);
    itemsSpacing = itemSize.width() * DOCK_ICON_SPACING;
    dockBorderRadius = itemSize.width() * 0.38;
    dockMargin = itemSize.width() * DOCK_PADDING_TOP;
    dockSize.setHeight(dockMargin + itemSize.height());

    dockWidget.separator.setFixedSize(
        itemSize.width() * DOCK_SEPARATOR_WIDTH,
        dockSize.height() - 2 * dockMargin);

    double dockWidth { dockMargin };

    for (QWidget *item : items)
    {
        item->move(dockWidth, dockMargin);

        if (!item->property("DockItem").isNull())
        {
            static_cast<DockItem*>(item)->updateDimensions();
            dockWidth += static_cast<DockItem*>(item)->controller.itemScale() * itemSize.width();
        }
        else
            dockWidth += itemSize.width() * DOCK_SEPARATOR_WIDTH;

        dockWidth += itemsSpacing;
    }

    dockWidth += dockMargin - itemsSpacing;

    dockWidget.setFixedHeight(dockSize.height());
    dockWidget.setBorderRadius(dockBorderRadius);

    if (dockWidth > width() - 16)
        dockWidth = width() - 16;

    dockWidget.setFixedWidth(dockWidth);
    dockWidget.move(
        (width() - dockWidget.width())/2,
        height() - dockSize.height() - DOCK_MARGIN_BOTTOM);
    dockWidget.hide();
    dockWidget.show();
}

double Dock::maxItemSize() noexcept
{
    double desiredItemSize { maxHeight * 0.65 * dockScale };

    n = 2.0 * DOCK_PADDING_TOP - DOCK_ICON_SPACING;

    for (QWidget *item : items)
    {
        if (!item->property("DockItem").isNull())
            n += static_cast<DockItem*>(item)->controller.itemScale();
        else
            n += DOCK_SEPARATOR_WIDTH;

        n += DOCK_ICON_SPACING;
    }

    double maxDockWidth = width() - 16;

    return desiredItemSize * n < maxDockWidth ? desiredItemSize : maxDockWidth / n;
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

void Dock::paintEvent(QPaintEvent *event)
{
    /*
    QPainter p(this);

    for (int i = 0; i < dockWidget.layout.count(); i++)
    {
        auto *it = dockWidget.layout.itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
        {
            DockItem *d {static_cast<DockItem*>(it->widget())};
            d->layout.update();
            QPoint pos = d->icon.mapTo(this, QPoint());

            p.drawPixmap(pos.x(), pos.y(), d->icon.width(), d->icon.height(), d->controller.icon());
        }
    }

    p.end();*/
}

void Dock::hideAllTooltips() noexcept
{
    int i { 0 };

    for (QWidget *it : items)
    {
        if (!it->property("DockItem").isNull())
            static_cast<DockItem*>(it)->tooltip.move(2 * i, 1000);

        i++;
    }
}


