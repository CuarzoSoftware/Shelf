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
                for (int i = 0; i < dockWidget.layout.count(); i++)
                {
                    auto *it = dockWidget.layout.itemAt(i);
                    if (it->widget() && !it->widget()->property("DockItem").isNull())
                    {
                        auto *item = static_cast<DockItem*>(it->widget());
                        item->tooltip.hide();
                        item->tooltip.label.setText(item->controller.name());
                        item->tooltip.show();
                        item->tooltip.windowHandle()->setParent(windowHandle());
                        item->tooltip.move(2 * i, height() - 10);
                        auto &caret = item->tooltip.caret;
                        caret.move(
                            item->tooltip.container.pos() + QPoint((item->tooltip.container.width() - caret.width())/2,
                            item->tooltip.container.size().height() - 2));

                    }
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

    layout.setContentsMargins(32, 32, 32, DOCK_MARGIN_BOTTOM);
    layout.addStretch(1);
    layout.addWidget(&dockWidget, 0, Qt::AlignHCenter);

    for (AppController *app : Shelf::get().pinnedApps)
        new DockItem(*app, *this);

    new DockItem((AppController&)*Shelf::get().trash, *this);

    show();
    updateDimensions();
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
        for (int i = 0; i < dockWidget.layout.count(); i++)
        {
            auto *it = dockWidget.layout.itemAt(i);
            if (it->widget() && !it->widget()->property("DockItem").isNull())
                static_cast<DockItem*>(it->widget())->tooltip.hide();
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

    if (!isDockVisible)
        setOffsetY(2 - height());
    else
        hideAllTooltips();
}

void Dock::updateDimensions() noexcept
{
    itemSize.setWidth(maxItemSize());
    dotSize = itemSize.width() * 0.08;

    double shadowM = screen()->devicePixelRatio() * itemSize.width() * 0.125;
    layout.setContentsMargins(0 , shadowM , shadowM , DOCK_MARGIN_BOTTOM);
    dockWidget.shadowFx.setBlurRadius(shadowM * 2.0);
    dockWidget.shadowFx.setOffset(0, shadowM * 0.2);
    dockAndMargins = itemSize.height() + dockWidget.shadowFx.blurRadius() + layout.contentsMargins().bottom();

    if (dotSize < 3)
        dotSize = 3;

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

    dockWidget.layout.setContentsMargins(dockMargins);
    dockWidget.layout.setSpacing(itemsSpacing);
    dockWidget.setFixedHeight(dockSize.height());
    dockWidget.setBorderRadius(dockBorderRadius);

    for (int i = 0; i < dockWidget.layout.count(); i++)
    {
        auto *it = dockWidget.layout.itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
            static_cast<DockItem*>(it->widget())->updateDimensions();
    }

    dockWidget.separator.setFixedHeight(dockWidget.height() - 2 * dockMargins.top());
}

int Dock::maxItemSize() const noexcept
{
    double N = dockWidget.layout.count() - 1;
    int maxWidth = double(screen()->size().width()) / (N + 0.18*N + 1.18);
    int desired = dockScale * 62;

    return qMin(qMin(maxWidth, desired), int(maxHeight - (8 + screen()->devicePixelRatio() *  maxHeight * 0.2)));
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

void Dock::hideAllTooltips() noexcept
{
    for (int i = 0; i < dockWidget.layout.count(); i++)
    {
        auto *it = dockWidget.layout.itemAt(i);
        if (it->widget() && !it->widget()->property("DockItem").isNull())
            static_cast<DockItem*>(it->widget())->tooltip.move(2 * i, 1000);
    }
}


