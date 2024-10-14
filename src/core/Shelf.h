#ifndef SHELF_H
#define SHELF_H

#include <wayland-client.h>
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
#include <include/core/SkColorFilter.h>

#include <core/Utils.h>

#include <QDir>

#include <memory>

class Dock;
class Seat;
class Window;
class Item;
class Popup;
struct wl_cursor;
struct wl_cursor_theme;
struct zwlr_layer_shell_v1;
struct xdg_wm_base;

enum ShelfCursor : int
{
    CursorArrow = 0,
    CursorNSResize = 1,
    CursorHidden
};

struct Cursor
{
    const char *name;
    wl_cursor *cursor;
    wl_surface *surface;
};

static Cursor cursors[]
{
    { "arrow", nullptr, nullptr },
    { "ns-resize", nullptr, nullptr },
};

class Shelf
{
public:
    static int fd() noexcept
    {
        return get().init();
    }

    static Shelf &get() noexcept
    {
        static Shelf shelf {};
        return shelf;
    }

    static void dispatch() noexcept
    {
        wl_display_dispatch(get().wlDisplay);
    }

    static int dispatchPending() noexcept
    {
        return wl_display_dispatch_pending(get().wlDisplay);
    }

    static void flush() noexcept
    {
        wl_display_flush(get().wlDisplay);
    }

    static void setCursor(ShelfCursor cursor) noexcept;

    Popup *popup { nullptr };
    Window *focusedWindow { nullptr };
    Item *focusedItem { nullptr };
    Item *draggedItem { nullptr };
    wl_pointer *currentWlPointer { nullptr };
    uint32_t pointerEnterSerial;
    SkPoint pointerPos { 0, 0 };
    uint32_t pointerButton;
    uint32_t pointerButtonState { WL_POINTER_BUTTON_STATE_RELEASED };

    bool running { false };

    wl_display *wlDisplay;
    wl_registry *wlRegistry;
    wl_compositor *wlCompositor { nullptr };
    wl_shm * wlShm { nullptr };
    wl_subcompositor *wlSubcompositor { nullptr };
    zwlr_layer_shell_v1 *wlLayerShell { nullptr };
    xdg_wm_base *xdgWmBase { nullptr };

    wl_region *wlEmptyRegion;
    wl_cursor_theme *wlCursorTheme { nullptr };
    ShelfCursor currentCursor { CursorHidden };

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;

    sk_sp<GrDirectContext> skContext;
    sk_sp<SkColorSpace> skColorSpace;
    MultiSizeImage defaultIcon;
    MultiSizeImage trashEmptyIcon;
    MultiSizeImage trashFullIcon;
    sk_sp<SkTypeface> typeface;
    SkFont tooltipFont;
    sk_sp<SkMaskFilter> tooltipBlurMaskFilter;
    sk_sp<SkColorFilter> darkenFilter = SkColorFilters::Blend(SkColorSetARGB(255, 128, 128, 128), SkBlendMode::kModulate);

    std::vector<QDir> appsDirs;

    std::list<Dock*> docks;
    std::list<Seat*> seats;
    std::list<AppController*> pinnedApps;
    std::list<AppController*> unpinnedApps;
    std::unique_ptr<SeparatorController> separator;
    std::unique_ptr<TrashController> trash;

    /* Dock color */
    SkColor4f dockGradientColors[2] {
        SkColor4f::FromColor(SkColorSetARGB(255, 64, 64, 64/*253, 244, 244, 244*/)),
        SkColor4f::FromColor(SkColorSetARGB(250, 64, 64, 64/*250, 235, 233, 238*/))
    };
    const float dockGradientPositions[2] { 0.0f, 1.0f };
private:
    Shelf() noexcept = default;
    int init() noexcept;
    bool initWayland() noexcept;
    bool initCursors() noexcept;
    bool initEGL() noexcept;
    bool initSkia() noexcept;
    void initDocks() noexcept;
    bool initConfig() noexcept;
};

#endif // SHELF_H
