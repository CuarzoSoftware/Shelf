#ifndef SHELF_H
#define SHELF_H

#include <wayland-client.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <EGL/egl.h>
#include <list>

class Dock;

class Shelf
{
public:
    static int run() noexcept
    {
        return get().init();
    }

    static Shelf &get() noexcept
    {
        static Shelf shelf {};
        return shelf;
    }

    std::list<Dock*> docks;

    bool running { false };

    wl_display *wlDisplay;
    wl_registry *wlRegistry;
    wl_compositor *wlCompositor { nullptr };
    wl_seat *wlSeat { nullptr };
    wl_pointer *wlPointer { nullptr };
    zwlr_layer_shell_v1 *wlLayerShell { nullptr };

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;
private:
    Shelf() noexcept = default;
    int init() noexcept;
    bool initWayland() noexcept;
    bool initEGL() noexcept;
    void initDocks() noexcept;
};

#endif // SHELF_H
