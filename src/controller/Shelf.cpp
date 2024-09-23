#include <qpa/qplatformnativeinterface.h>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QScreen>

#include <KDesktopFile>

#include <controller/Shelf.h>
#include <widgets/Dock.h>

static const zwlr_foreign_toplevel_manager_v1_listener foreignManagerListener
{
    .toplevel = [](void *data, zwlr_foreign_toplevel_manager_v1 *manager, zwlr_foreign_toplevel_handle_v1 *toplevel)
    {
        qInfo() << "New Toplevel";
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

        if (!app.foreignToplevelManager && strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
        {
            app.foreignToplevelManager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
                wl_registry_bind(wl_registry, name, &zwlr_foreign_toplevel_manager_v1_interface, version));

            zwlr_foreign_toplevel_manager_v1_add_listener(app.foreignToplevelManager, &foreignManagerListener, &app);
        }
    },
    .global_remove = [](void *data, wl_registry *wl_registry, uint32_t name)
    {

    }
};

Shelf::Shelf(int &argc, char **argv) : QApplication(argc, argv)
{
    display = (wl_display*)platformNativeInterface()->nativeResourceForIntegration("wl_display");

    if (!display)
    {
        qFatal() << "Could not get wl_display. Try running it with --platform wayland.";
        exit(EXIT_FAILURE);
    }

    loadConfig();

    connect(this, &Shelf::screenAdded, this, &Shelf::onScreenAdded);
    connect(this, &Shelf::screenRemoved, this, &Shelf::onScreenRemoved);

    for (auto *screen : screens())
        onScreenAdded(screen);

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);

    if (!foreignToplevelManager)
    {
        qFatal() << "Could not bind to zwlr_foreign_toplevel_manager_v1.";
        exit(EXIT_FAILURE);
    }
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
