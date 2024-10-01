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
    m_icon(icon),
    m_running(running)
{
    m_jumpAnim->setDuration(500);
    m_jumpAnim->setStartValue(0.0);
    m_jumpAnim->setEndValue(1.0);
    m_jumpAnim->setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));

    connect(m_jumpAnim, &QPropertyAnimation::finished, [this](){

        if (jumpPhase() == 0.0)
        {
            if (!iconsJumping())
            {
                for (DockItem *item : m_items)
                {
                    item->icon->show();
                    item->iconSubstitute->hide();

                    if (item->dock.isDockVisible)
                        item->tooltip->showName(item);

                    QTimer::singleShot(1, [item](){
                        item->jumpingIconContainer->hide();
                    });
                }

                return;
            }

            m_jumpAnim->setDuration(500);
            m_jumpAnim->setStartValue(0.0);
            m_jumpAnim->setEndValue(1.0);
            m_jumpAnim->setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));
        }
        else
        {
            m_jumpAnim->setDuration(500);
            m_jumpAnim->setStartValue(1.0);
            m_jumpAnim->setEndValue(0.0);
            m_jumpAnim->setEasingCurve(QEasingCurve(QEasingCurve::InQuad));
        }

        m_jumpAnim->start();

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

const QString &ItemController::iconName() const noexcept
{
    return m_iconName;
}

const QPixmap &ItemController::icon() const noexcept
{
    return m_icon;
}

void ItemController::setIcon(const QPixmap &icon) noexcept
{
    m_icon = icon;

    for (DockItem *item : m_items)
    {
        item->icon->setPixmap(m_icon);
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
            item->jumpingIcon->setPixmap(item->controller.icon());
            item->jumpingIcon->move(16, 16);
            item->jumpingIcon->show();
            item->jumpingIconContainer->move(item->iconSubstitute->mapTo(&item->dock, QPoint(-16, -16)));
            item->jumpingIconContainer->show();
            item->iconSubstitute->show();
            item->icon->hide();

            if (item->dock.isDockVisible)
                item->tooltip->showName(item);
        }
        m_jumpAnim->setDuration(500);
        m_jumpAnim->setStartValue(0.0);
        m_jumpAnim->setEndValue(1.0);
        m_jumpAnim->setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));
        m_jumpAnim->start();
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
    jumpPhaseChanged();

    for (DockItem *item : m_items)
    {
        QPoint newPos = item->iconSubstitute->mapTo(&item->dock, QPoint(-16, -16));
        newPos.setY(newPos.y() - phase * item->dock.itemSize.height() * 0.5);
        item->jumpingIconContainer->move(newPos);

        if (item->mouseIsOver)
        {
            const QPoint itemPos { item->mapTo(&item->dock, QPoint(0, -phase * item->dock.itemSize.height() * 0.5)) };
            item->tooltip->move(itemPos.x() + (item->width() - item->tooltip->width())/2, itemPos.y() -item->tooltip->height() - item->dock.tooltipDistY);
        }
    }
}

double ItemController::jumpPhase() const noexcept
{
    return m_jumpPhase;
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
        item->dot->setVisible(running);
}

ItemController::Type ItemController::type() const noexcept
{
    return m_type;
}
