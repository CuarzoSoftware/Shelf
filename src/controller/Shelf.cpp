#include <protocols/wlr-foreign-toplevel-management-unstable-v1-client.h>
#include <wayland-client.h>

#include <qpa/qplatformnativeinterface.h>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QScreen>
#include <QPainter>

#include <KDesktopFile>

#include <controller/Shelf.h>
#include <controller/ToplevelHandle.h>
#include <controller/ItemController.h>
#include <controller/TrashController.h>
#include <widgets/Dock.h>
#include <widgets/Tooltip.h>

static const zwlr_foreign_toplevel_manager_v1_listener foreignManagerListener
{
    .toplevel = [](void *data, zwlr_foreign_toplevel_manager_v1 *manager, zwlr_foreign_toplevel_handle_v1 *toplevel)
    {
        qInfo() << "New Toplevel";
        new ToplevelHandle(toplevel);
    },
    .finished = [](void *data, zwlr_foreign_toplevel_manager_v1 *manager)
    {
        qInfo() << "Finished";
    }
};

static const wl_registry_listener registryListener
{
    .global = [](void *data, wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
    {
        auto &app { *static_cast<Shelf*>(data) };

        if (!app.toplevelManager && strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
        {
            if (version < 2)
            {
                qFatal() << "zwlr_foreign_toplevel_manager_v1 >= 2 is required.";
                exit(1);
            }

            app.toplevelManager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
                wl_registry_bind(wl_registry, name, &zwlr_foreign_toplevel_manager_v1_interface, 2));

            zwlr_foreign_toplevel_manager_v1_add_listener(app.toplevelManager, &foreignManagerListener, &app);
        }
    },
    .global_remove = [](void *data, wl_registry *wl_registry, uint32_t name)
    {

    }
};

Shelf::Shelf(int &argc, char **argv) : QApplication(argc, argv)
{
    initCreateUIElements();

    appsDirs.reserve(16); // XDG_DATA_DIRS/applications

    for (const QString &appDir : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation))
        appsDirs.emplaceBack(appDir);

    QIcon::setThemeName("WhiteSur");
    QIcon::setFallbackThemeName("Paper");

    display = (wl_display*)platformNativeInterface()->nativeResourceForIntegration("wl_display");

    if (!display)
    {
        qFatal() << "Could not get wl_display. Try running it with --platform wayland.";
        exit(EXIT_FAILURE);
    }

    defaultAppIcon = iconFromName("terminal", 512);

    loadConfig();

    connect(this, &Shelf::screenAdded, this, &Shelf::onScreenAdded);
    connect(this, &Shelf::screenRemoved, this, &Shelf::onScreenRemoved);
    for (auto *screen : screens()) onScreenAdded(screen);

    new TrashController();

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);

    if (!toplevelManager)
    {
        qFatal() << "Could not bind to zwlr_foreign_toplevel_manager_v1.";
        exit(EXIT_FAILURE);
    }
}

void Shelf::initCreateUIElements() noexcept
{
    dot = QPixmap(8,8);
    dot.fill(Qt::transparent);
    QPainter p(&dot);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setBrush(QColor(123, 123, 123));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, dot.width(), dot.height());
}

void Shelf::loadConfig() noexcept
{
    auto homeLocations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

    if (homeLocations.empty())
    {
        qFatal() << "Could not get HOME dir.";
        exit(EXIT_FAILURE);
    }

    homeDir = QDir(homeLocations.front());

    QString val;
    QFile file;
    file.setFileName(homeDir.filePath(".config/Cuarzo/Shelf/config.json"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();

    QJsonDocument conf = QJsonDocument::fromJson(val.toUtf8());
    QJsonArray items = conf["items"].toArray();


    for (const auto &item : items)
    {
        KDesktopFile df("Hola");//item.toString());

        qInfo() << df.fileName();
    }
}

void Shelf::onScreenAdded(QScreen *screen) noexcept
{
    for (Dock *dock : docks)
        if (dock->screen() == screen)
            return;

    docks.emplaceBack(new Dock(screen));
}

void Shelf::onScreenRemoved(QScreen *screen) noexcept
{
    for (size_t i = 0; i < docks.size(); i++)
    {
        if (docks[i]->screen() == screen)
        {
            delete docks[i];
            docks[i] = docks[docks.size() - 1];
            docks.pop_back();
            return;
        }
    }
}

void Shelf::makeWidgetTransparent(QWidget *widget) noexcept
{
    widget->setPalette(Qt::transparent);
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_TranslucentBackground);
}

QString Shelf::desktopFileFromAppId(const QString &appId) noexcept
{
    // TODO handle / cases

    QString filePath;
    const QString fileName { appId.trimmed() + ".desktop" };
    for (const QDir &appDir : get().appsDirs)
    {
        filePath = appDir.filePath(fileName);

        if (QFile::exists(filePath))
            return filePath;
    }

    return QString();
}

QPixmap Shelf::iconFromName(const QString &name, int size) noexcept
{
    QIcon icon = QIcon::fromTheme(name);

    if (icon.isNull())
        return QPixmap();

    return QPixmap::fromImage(icon.pixmap(size, size).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
}
