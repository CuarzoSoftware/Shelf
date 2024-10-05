#include <controllers/ItemController.h>
#include <items/Item.h>
#include <windows/Dock.h>

Item::Item(ItemController &controller, Dock &dock) noexcept :
    m_controller(controller),
    m_dock(dock)
{
    controller.m_items.emplace_back(this);
    m_controllerLink = std::prev(controller.m_items.end());

    dock.m_items.emplace_back(this);
    m_dockLink = std::prev(dock.m_items.end());

    dock.update();
}

Item::~Item() noexcept
{
    dock().m_items.erase(m_dockLink);
    controller().m_items.erase(m_controllerLink);
}

void Item::paintEvent(SkCanvas *c) noexcept
{
    controller().drawItem(c, *this);
}
