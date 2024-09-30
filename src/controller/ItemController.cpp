#include <controller/ItemController.h>
#include <controller/Shelf.h>
#include <widgets/DockItem.h>

ItemController::ItemController(Type type, const QString &name, const QPixmap &icon, bool active) noexcept :
    m_type(type),
    m_name(name),
    m_icon(icon),
    m_active(active)
{
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

bool ItemController::active() const noexcept
{
    return m_active;
}

void ItemController::setActive(bool active) noexcept
{
    if (active == m_active)
        return;

    for (DockItem *item : m_items)
        item->dot->setVisible(active);
}

ItemController::Type ItemController::type() const noexcept
{
    return m_type;
}
