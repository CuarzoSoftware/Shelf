#include <controllers/ItemController.h>
#include <items/Item.h>
#include <core/Utils.h>
#include <QPixmap>

ItemController::ItemController(Type type, bool running, const QString &name, const QPixmap &icon, const QString &iconName) noexcept :
    m_type(type),
    m_running(running),
    m_name(name),
    m_iconName(iconName)
{
    if (icon.isNull())
        m_icon = shelf().defaultIcon;
    else
        m_icon = Utils::skImageFromPixmap(icon);

    for (Dock *dock : shelf().docks)
        new Item(*this, *dock);
}
