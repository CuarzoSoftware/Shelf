#ifndef SHELF_H
#define SHELF_H

#include <QApplication>
#include <QDir>
#include <QVector>
#include <wayland-client.h>
#include <protocols/wlr-foreign-toplevel-management-unstable-v1-client.h>

class Dock;

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

    QDir homeDir;
    wl_display *display;
    wl_registry *registry;
    zwlr_foreign_toplevel_manager_v1 *foreignToplevelManager { nullptr };

    QVector<Dock*> docks;
};

#endif // SHELF_H
