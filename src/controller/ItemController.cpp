#include <controller/ItemController.h>
#include <controller/Shelf.h>
#include <widgets/DockItem.h>
#include <widgets/Dock.h>
#include <QtMath>
#include <QTimer>

ItemController::ItemController(Type type, const QString &name, const QPixmap &icon, bool running, const QString &iconName) noexcept :
    m_type(type),
    m_name(name),
    m_iconName(iconName),
    m_icon(Shelf::applyShadow(icon, ICON_SIZE)),
    m_iconDark(Shelf::darkerIcon(m_icon)),
    m_running(running)
{
    m_jumpAnim.setDuration(ICON_JUMP_MS);
    m_jumpAnim.setStartValue(0.0);
    m_jumpAnim.setEndValue(1.0);
    m_jumpAnim.setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));

    connect(&m_jumpAnim, &QPropertyAnimation::finished, [this](){

        if (jumpPhase() == 0.0)
        {
            if (!iconsJumping())
            {
                for (DockItem *item : m_items)
                {
                    item->icon.show();
                    item->iconSubstitute.hide();

                    if (item->dock.isDockVisible)
                        item->tooltip.showItemName(item);

                    QTimer::singleShot(1, [item](){
                        item->jumpingIcon.hide();
                    });
                }

                return;
            }

            m_jumpAnim.setDuration(ICON_JUMP_MS);
            m_jumpAnim.setStartValue(0.0);
            m_jumpAnim.setEndValue(1.0);
            m_jumpAnim.setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));
        }
        else
        {
            m_jumpAnim.setDuration(ICON_JUMP_MS);
            m_jumpAnim.setStartValue(1.0);
            m_jumpAnim.setEndValue(0.0);
            m_jumpAnim.setEasingCurve(QEasingCurve(QEasingCurve::InQuad));
        }

        m_jumpAnim.start();

    });

    for (auto *dock : Shelf::get().docks)
        new DockItem(*this, *dock);
}

ItemController::~ItemController() noexcept
{
    while (!m_items.empty())
        delete m_items.back();
}

std::list<DockItem *> &ItemController::items() noexcept
{
    return m_items;
}

const QString &ItemController::name() const noexcept
{
    return m_name;
}

void ItemController::setName(const QString &name) noexcept
{
    if (m_name != name)
    {
        m_name = name;
    }
}

void ItemController::setIcon(const QPixmap &icon) noexcept
{
    m_icon = icon;
    m_iconDark = Shelf::darkerIcon(icon);

    for (DockItem *item : m_items)
    {
        item->icon.setPixmap(m_icon);
        item->updateDimensions();
    }
}

void ItemController::setIconsJumping(bool jumping) noexcept
{
    if (jumping == m_iconsJumping)
        return;

    m_iconsJumping = jumping;

    if (jumping)
    {
        for (DockItem *item : m_items)
        {
            item->jumpingIcon.move(item->iconSubstitute.mapTo(&item->dock, QPoint()));
            item->jumpingIcon.show();
            item->jumpingIcon.raise();
            item->jumpingIcon.update();

            if (item->dock.isDockVisible)
                item->tooltip.showItemName(item);
        }
        m_jumpAnim.setDuration(ICON_JUMP_MS);
        m_jumpAnim.setStartValue(0.0);
        m_jumpAnim.setEndValue(1.0);
        m_jumpAnim.setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));
        m_jumpAnim.start();
    }
}

bool ItemController::iconsJumping() const noexcept
{
    return m_iconsJumping;
}

void ItemController::setJumpPhase(double phase) noexcept
{
    if (m_jumpPhase == phase)
        return;

    m_jumpPhase = phase;

    for (DockItem *item : m_items)
    {
        QPoint newPos = item->iconSubstitute.mapTo(&item->dock, QPoint());
        newPos.setY(newPos.y() - phase * item->dock.itemSize.height() * ICON_JUMP_HEIGHT);
        item->jumpingIcon.move(newPos);

        if (item->mouseIsOver)
        {
            const QPoint itemPos { item->mapTo(&item->dock, QPoint(0, -phase * item->dock.itemSize.height() * ICON_JUMP_HEIGHT)) };
            item->tooltip.move(itemPos.x() + (item->width() - item->tooltip.width())/2, itemPos.y() -item->tooltip.height() - item->dock.tooltipDistY);
        }
    }
}

double ItemController::jumpPhase() const noexcept
{
    return m_jumpPhase;
}

void ItemController::setItemScale(double scale) noexcept
{
    if (m_itemScale == scale)
        return;

    m_itemScale = scale;

    for (DockItem *item : m_items)
        item->updateDimensions();
}

void ItemController::startScaleAnim(double from, double to) noexcept
{
    m_scaleAnim.setEasingCurve(QEasingCurve(QEasingCurve::InQuad));
    m_scaleAnim.setDuration(300);
    m_scaleAnim.setStartValue(from);
    m_scaleAnim.setEndValue(to);
    m_scaleAnim.start();
}

bool ItemController::running() const noexcept
{
    return m_running;
}

void ItemController::setRunning(bool running) noexcept
{
    if (running == m_running)
        return;

    m_running = running;

    for (DockItem *item : m_items)
        item->dot.setVisible(running);
}

ItemController::Type ItemController::type() const noexcept
{
    return m_type;
}
