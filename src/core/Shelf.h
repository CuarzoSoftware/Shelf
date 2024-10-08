#ifndef SHELF_H
#define SHELF_H

#include <wayland-client.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <EGL/egl.h>
#include <list>

#include <controllers/AppController.h>
#include <controllers/SeparatorController.h>
#include <controllers/TrashController.h>

#include <include/gpu/GrDirectContext.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFont.h>
#include <include/core/SkMaskFilter.h>

#include <QDir>

#include <memory>

class Dock;
class Seat;
class Window;
class Item;

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

    Window *focusedWindow { nullptr };
    Item *focusedItem { nullptr };
    SkPoint pointerPos { 0, 0 };

    bool running { false };

    wl_display *wlDisplay;
    wl_registry *wlRegistry;
    wl_compositor *wlCompositor { nullptr };
    wl_subcompositor *wlSubcompositor { nullptr };
    zwlr_layer_shell_v1 *wlLayerShell { nullptr };
    wl_region *wlEmptyRegion;

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;

    sk_sp<GrDirectContext> skContext;
    sk_sp<SkColorSpace> skColorSpace;
    sk_sp<SkImage> defaultIcon;
    sk_sp<SkImage> trashEmptyIcon;
    sk_sp<SkImage> trashFullIcon;
    sk_sp<SkTypeface> typeface;
    SkFont tooltipFont;
    sk_sp<SkMaskFilter> tooltipBlurMaskFilter;

    std::vector<QDir> appsDirs;

    std::list<Dock*> docks;
    std::list<Seat*> seats;
    std::list<AppController*> pinnedApps;
    std::list<AppController*> unpinnedApps;
    std::unique_ptr<SeparatorController> separator;
    std::unique_ptr<TrashController> trash;
private:
    Shelf() noexcept = default;
    int init() noexcept;
    bool initWayland() noexcept;
    bool initEGL() noexcept;
    bool initSkia() noexcept;
    void initDocks() noexcept;
    bool initConfig() noexcept;
};

#endif // SHELF_H
