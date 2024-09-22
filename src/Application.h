#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <wayland-client.h>
#include <protocols/wlr-foreign-toplevel-management-unstable-v1-client.h>

class Application final : public QApplication
{
public:
    Application(int &argc, char **argv);

    wl_display *display;
    wl_registry *registry;
    zwlr_foreign_toplevel_manager_v1 *foreignToplevelManager { nullptr };
};

#endif // APPLICATION_H
