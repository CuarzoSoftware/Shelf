#ifndef SHELF_H
#define SHELF_H

#include <QApplication>
#include <QDir>
#include <QVector>
#include <list>

class Dock;
class ToplevelHandle;
class ItemController;
class AppController;
class TrashController;
struct zwlr_foreign_toplevel_manager_v1;
struct wl_display;
struct wl_registry;

class Shelf final : public QApplication
{
public:
    Shelf(int &argc, char **argv);

    void initCreateUIElements() noexcept;
    void loadConfig() noexcept;

    static Shelf &get()
    {
        return *static_cast<Shelf*>(QApplication::instance());
    }

    void onScreenAdded(QScreen *screen) noexcept;
    void onScreenRemoved(QScreen *screen) noexcept;

    static void makeWidgetTransparent(QWidget *widget) noexcept;
    static QString desktopFileFromAppId(const QString &appId) noexcept;
    static QString appIdFromDesktopFile(const QString &desktopFilePath) noexcept;
    static QPixmap iconFromName(const QString &name, int size) noexcept;
    static QPixmap applyShadow(const QPixmap &pixmap, int size) noexcept;
    static QPixmap darkerIcon(const QPixmap &pixmap) noexcept;
    static QPixmap createThumbnail(const QPixmap &pixmap) noexcept;

    QVector<QDir> appsDirs;
    QVector<Dock*> docks;

    // Items
    std::list<AppController*> pinnedApps;
    std::list<AppController*> unpinnedApps;
    std::list<ItemController*> dirs;
    std::list<ItemController*> thumbnails;
    TrashController *trash;

    // Icons
    QPixmap defaultAppIcon;
    QPixmap dot;

    // Wayland
    wl_display *display;
    wl_registry *registry;
    wl_seat *seat;
    zwlr_foreign_toplevel_manager_v1 *toplevelManager { nullptr };
    std::list<ToplevelHandle*> toplevelHandles;
};

#endif // SHELF_H
