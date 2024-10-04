#ifndef DOCK_H
#define DOCK_H

#include <list>
#include <inttypes.h>
#include <EGL/egl.h>
#include <include/core/SkSize.h>
#include <include/core/SkSurface.h>

struct wl_surface;
struct wl_output;
struct wl_callback;
struct wl_egl_window;
struct zwlr_layer_surface_v1;

class Dock
{
public:
    Dock(wl_output *wlOutput, uint32_t name) noexcept;
    ~Dock() noexcept;

    void updateSkSurface() noexcept;
    void render() noexcept;

    float scale { 1.f };
    SkSize size { 1, 200 };
    SkSize bufferSize { size };

    sk_sp<SkSurface> skSurface;

    wl_callback *wlCallback { nullptr };
    wl_output *wlOutput;
    EGLSurface eglSurface { EGL_NO_SURFACE };
    wl_surface *wlSurface { nullptr };
    wl_egl_window *wlEGLWindow { nullptr };
    zwlr_layer_surface_v1 *wlLayerSurface { nullptr };
    std::list<Dock*>::iterator shelfLink;
    uint32_t name;
};

#endif // DOCK_H
