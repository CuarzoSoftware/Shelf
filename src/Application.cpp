#include <Application.h>
#include <qpa/qplatformnativeinterface.h>

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
        auto &app { *static_cast<Application*>(data) };

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

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    display = (wl_display*)platformNativeInterface()->nativeResourceForIntegration("wl_display");

    if (!display)
    {
        qFatal() << "Could not get wl_display. Try running it with --platform wayland.";
        exit(EXIT_FAILURE);
    }

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);

    if (!foreignToplevelManager)
    {
        qFatal() << "Could not bind to zwlr_foreign_toplevel_manager_v1.";
        exit(EXIT_FAILURE);
    }
}
