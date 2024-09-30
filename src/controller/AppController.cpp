#include <controller/ToplevelHandle.h>
#include <controller/AppController.h>
#include <controller/Shelf.h>

AppController::AppController(const QString &iconName, const QString &name, const QString &id, const QString &desktopFilePath, bool pinned, bool active) noexcept :
    ItemController(App, name, Shelf::iconFromName(iconName, 512), active),
    m_id(id),
    m_desktopFilePath(desktopFilePath),
    m_pinned(false)
{
    if (m_pinned)
    {
        Shelf::get().pinnedApps.push_back(this);
        shelfLink = std::prev(Shelf::get().pinnedApps.end());
    }
    else
    {
        Shelf::get().unpinnedApps.push_back(this);
        shelfLink = std::prev(Shelf::get().unpinnedApps.end());
    }

    if (icon().isNull())
        setIcon(Shelf::get().defaultAppIcon);
}

AppController::~AppController() noexcept
{
    if (m_pinned)
        Shelf::get().pinnedApps.erase(shelfLink);
    else
        Shelf::get().unpinnedApps.erase(shelfLink);
}

bool AppController::isPinnable() const noexcept
{
    return !m_desktopFilePath.isNull();
}

void AppController::addToplevelHandle(ToplevelHandle *handle) noexcept
{
    m_toplevelHandles.push_back(handle);
    handle->appControllerLink = std::prev(m_toplevelHandles.end());
    handle->appController = this;
}

void AppController::removeToplevelHandle(ToplevelHandle *handle) noexcept
{
    m_toplevelHandles.erase(handle->appControllerLink);
    handle->appController = nullptr;

    if (!m_pinned && m_toplevelHandles.empty())
        delete this;
}

const std::list<ToplevelHandle *> &AppController::toplevelHandles() const noexcept
{
    return m_toplevelHandles;
}

const QString &AppController::id() const noexcept
{
    return m_id;
}

void AppController::onItemClick(DockItem *item) noexcept
{
    if (!toplevelHandles().empty())
    {
        toplevelHandles().back()->sendMinimized(false);
        toplevelHandles().back()->sendActivate();
    }
}
