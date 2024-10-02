#include <controller/Constants.h>
#include <controller/ToplevelHandle.h>
#include <controller/AppController.h>
#include <controller/Shelf.h>
#include <QProcess>

AppController::AppController(const QString &iconName, const QString &name, const QString &id, const QString &desktopFilePath, bool pinned, bool active) noexcept :
    ItemController(App, name, Shelf::iconFromName(iconName, 128), active, iconName),
    m_id(id),
    m_desktopFilePath(desktopFilePath),
    m_pinned(pinned)
{
    int64_t i = m_id.lastIndexOf(".");

    if (i != -1)
        m_idName = m_id.sliced(i+1).toLower();
    else
        m_idName = m_id.toLower();

    qInfo() << "ID NAME" << m_idName;

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

    m_launchTimeout.setInterval(APP_LAUNCH_TIMEOUT_MS);
    connect(&m_launchTimeout, &QTimer::timeout, [this](){
        setIconsJumping(false);
    });
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

    if (m_toplevelHandles.size() == 1)
    {
        setRunning(true);
        setIconsJumping(false);
    }
}

void AppController::removeToplevelHandle(ToplevelHandle *handle) noexcept
{
    m_toplevelHandles.erase(handle->appControllerLink);
    handle->appController = nullptr;

    if (m_toplevelHandles.empty())
    {
        if (!m_pinned)
            delete this;
        else
        {
            qInfo() << "App closed";
            setRunning(false);
        }
    }
}

const std::list<ToplevelHandle *> &AppController::toplevelHandles() const noexcept
{
    return m_toplevelHandles;
}

const QString &AppController::id() const noexcept
{
    return m_id;
}

const QString &AppController::idName() const noexcept
{
    return m_idName;
}

void AppController::onItemClick(DockItem *item) noexcept
{
    if (!toplevelHandles().empty())
    {
        toplevelHandles().back()->sendMinimized(false);
        toplevelHandles().back()->sendActivate();
    }
    else
    {
        const char *env = getenv("QT_WAYLAND_SHELL_INTEGRATION");
        unsetenv("QT_WAYLAND_SHELL_INTEGRATION");
        qInfo() << "Launching" << m_desktopFilePath;
        QProcess::startDetached("gio", QStringList() << "launch" << m_desktopFilePath);
        setenv("QT_WAYLAND_SHELL_INTEGRATION", env, 1);
        setIconsJumping(true);
        m_launchTimeout.start();
    }
}
