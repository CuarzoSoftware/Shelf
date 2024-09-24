#ifndef SHELF_H
#define SHELF_H

#include <QApplication>
#include <QDir>
#include <QVector>
#include <list>

class Dock;
class ToplevelHandle;
class ItemController;
class TrashController;
struct zwlr_foreign_toplevel_manager_v1;
struct wl_display;
struct wl_registry;

class Shelf final : public QApplication
{
public:
    Shelf(int &argc, char **argv);
    void loadConfig() noexcept;

    static Shelf &get()
    {
        return *static_cast<Shelf*>(QApplication::instance());
    }

    void onScreenAdded(QScreen *screen) noexcept;
    void onScreenRemoved(QScreen *screen) noexcept;

    static void makeWidgetTransparent(QWidget *widget) noexcept;
    static QString desktopFileFromAppId(const QString &appId) noexcept;
    static QPixmap iconFromName(const QString &name, int size) noexcept;

    QDir homeDir;
    QVector<QDir> appsDirs;
    QVector<Dock*> docks;

    // Items
    std::list<ItemController*> pinnedApps;
    std::list<ItemController*> unpinnedApps;
    std::list<ItemController*> dirs;
    std::list<ItemController*> thumbnails;
    TrashController *trash;

    // Wayland
    wl_display *display;
    wl_registry *registry;
    zwlr_foreign_toplevel_manager_v1 *toplevelManager { nullptr };
    std::list<ToplevelHandle*> toplevelHandles;
};

#endif // SHELF_H
