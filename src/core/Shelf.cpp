#include <include/gpu/gl/GrGLAssembleInterface.h>
#include <QStandardPaths>
#include <QIcon>
#include <QDebug>

#include <controllers/SeparatorController.h>

#include <core/Utils.h>
#include <core/Shelf.h>
#include <core/Constants.h>
#include <windows/Dock.h>
#include <input/Seat.h>

#include <protocols/xdg-shell.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>

static const xdg_wm_base_listener xdgWmBaseListener
{
    .ping = [](void *data, xdg_wm_base *xdgWmBase, uint32_t serial)
    {
        xdg_wm_base_pong(xdgWmBase, serial);
    }
};

static const wl_registry_listener wlRegistryListener {
.global = [](void *data, wl_registry *wlRegistry, uint32_t name, const char *interface, uint32_t version)
{
    auto &shelf { *static_cast<Shelf*>(data) };

    if (!shelf.wlCompositor && version >= 6 && strcmp(interface, wl_compositor_interface.name) == 0)
    {
        shelf.wlCompositor = (wl_compositor*)wl_registry_bind(wlRegistry, name, &wl_compositor_interface, 6);
        return;
    }

    if (!shelf.wlSubcompositor && strcmp(interface, wl_subcompositor_interface.name) == 0)
    {
        shelf.wlSubcompositor = (wl_subcompositor*)wl_registry_bind(wlRegistry, name, &wl_subcompositor_interface, 1);
        return;
    }

    if (!shelf.wlLayerShell && strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
        shelf.wlLayerShell = (zwlr_layer_shell_v1*)wl_registry_bind(wlRegistry, name, &zwlr_layer_shell_v1_interface, 1);
        return;
    }

    if (!shelf.wlShm && strcmp(interface, wl_shm_interface.name) == 0)
    {
        shelf.wlShm = (wl_shm*)wl_registry_bind(wlRegistry, name, &wl_shm_interface, 1);
        return;
    }

    if (!shelf.xdgWmBase && strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        shelf.xdgWmBase = (xdg_wm_base*)wl_registry_bind(wlRegistry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(shelf.xdgWmBase, &xdgWmBaseListener, NULL);
        return;
    }

    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        new Seat((wl_seat*)wl_registry_bind(wlRegistry, name, &wl_seat_interface, 1), name);
        return;
    }

    if (shelf.running && strcmp(interface, wl_output_interface.name) == 0)
    {
        new Dock((wl_output*)wl_registry_bind(wlRegistry, name, &wl_output_interface, 1), name);
        return;
    }

    /*
    if (!app.toplevelManager && strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
    {
        if (version < 2)
        {
            qFatal() << "zwlr_foreign_toplevel_manager_v1 >= 2 is required.";
            exit(1);
        }

        app.toplevelManager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
            wl_registry_bind(wl_registry, name, &zwlr_foreign_toplevel_manager_v1_interface, 2));

        zwlr_foreign_toplevel_manager_v1_add_listener(app.toplevelManager, &foreignManagerListener, &app);
    }*/
},
.global_remove = [](void *data, wl_registry *wl_registry, uint32_t name)
{
    auto &shelf { *static_cast<Shelf*>(data) };

    for (Dock *dock : shelf.docks)
    {
        if (dock->name == name)
        {
            delete dock;
            return;
        }
    }

    for (Seat *seat : shelf.seats)
    {
        if (seat->name == name)
        {
            delete seat;
            return;
        }
    }
}};

void Shelf::setCursor(ShelfCursor cursor) noexcept
{
    if (!get().currentWlPointer)
        return;

    if (cursor == get().currentCursor)
        return;
    else if (cursor == CursorHidden)
    {
        get().currentCursor = cursor;

        wl_pointer_set_cursor(
            get().currentWlPointer,
            get().pointerEnterSerial,
            NULL, 0, 0);
    }
    else
    {
        get().currentCursor = cursor;

        wl_pointer_set_cursor(
            get().currentWlPointer,
            get().pointerEnterSerial,
            cursors[cursor].surface,
            cursors[cursor].cursor->images[0]->hotspot_x * 0.5f,
            cursors[cursor].cursor->images[0]->hotspot_y * 0.5f);
    }
}

int Shelf::init() noexcept
{
    if (!initWayland())
        return -1;

    if (!initCursors())
        return -1;

    if (!initEGL())
        return -1;

    if (!initSkia())
        return -1;

    initConfig();

    running = true;

    /* Get wl_outputs */
    initDocks();

    flush();

    return wl_display_get_fd(wlDisplay);
}

bool Shelf::initWayland() noexcept
{
    wlDisplay = wl_display_connect(NULL);

    if (!wlDisplay)
    {
        qFatal() << "Could not get wl_display.";
        return false;
    }

    wlRegistry = wl_display_get_registry(wlDisplay);
    wl_registry_add_listener(wlRegistry, &wlRegistryListener, this);
    wl_display_roundtrip(wlDisplay);

    if (!wlCompositor)
    {
        qFatal() << "Could not get wl_compositor version 6.";
        return false;
    }

    if (!wlSubcompositor)
    {
        qFatal() << "Could not get wl_subcompositor version 1.";
        return false;
    }

    if (!wlLayerShell)
    {
        qFatal() << "Could not get zwlr_layer_shell_v1 version 1.";
        return false;
    }

    if (!wlShm)
    {
        qFatal() << "Could not get wl_shm version 1.";
        return false;
    }

    if (!xdgWmBase)
    {
        qFatal() << "Could not get xdg_wm_base version 1.";
        return false;
    }

    if (seats.empty())
    {
        qFatal() << "Could not get a wl_seat with version 1.";
        return false;
    }

    wlEmptyRegion = wl_compositor_create_region(wlCompositor);

    return true;
}

bool Shelf::initCursors() noexcept
{
    wlCursorTheme = wl_cursor_theme_load(NULL, 48, wlShm);

    if (!wlCursorTheme)
    {
        qFatal() << "Failed to load wl_cursor_theme.";
        return false;
    }

    for (std::size_t i = 0; i < sizeof(cursors)/sizeof(*cursors); i++)
    {
        cursors[i].cursor = wl_cursor_theme_get_cursor(wlCursorTheme, cursors[i].name);

        if (!cursors[i].cursor)
        {
            qFatal() << "Failed to load" << cursors[i].name << "wl_cursor.";
            return false;
        }

        qInfo() << "Found cursor:" << cursors[i].name;

        cursors[i].surface = wl_compositor_create_surface(wlCompositor);
        wl_surface_attach(
            cursors[i].surface,
            wl_cursor_image_get_buffer(cursors[i].cursor->images[0]),
            0, 0);
        wl_surface_set_buffer_scale(cursors[i].surface, 2);
        wl_surface_commit(cursors[i].surface);
    }

    return true;
}

bool Shelf::initEGL() noexcept
{
    const EGLint configAttribs[]
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    const EGLint contextAttribs[]
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint numConfigs;

    eglDisplay = eglGetDisplay((EGLNativeDisplayType)wlDisplay);

    if (eglDisplay == EGL_NO_DISPLAY)
    {
        qFatal() << "Failed to get EGL display.";
        return false;
    }

    if (!eglInitialize(eglDisplay, NULL, NULL))
    {
        qFatal() << "Failed to initialize EGL display.";
        return false;
    }

    eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs);

    if (numConfigs != 1)
    {
        qFatal() << "Failed to choose EGL config.";
        return false;
    }

    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);

    if (eglContext == EGL_NO_CONTEXT)
    {
        qFatal() << "Failed to create EGL context.";
        return false;
    }

    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);
    return true;
}

bool Shelf::initSkia() noexcept
{
    auto interface = GrGLMakeAssembledInterface(nullptr, (GrGLGetProc)*[](void *, const char *p) -> void * {
        return (void *)eglGetProcAddress(p);
    });

    GrContextOptions options;
    options.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kSkSL;
    options.fAvoidStencilBuffers = true;
    options.fPreferExternalImagesOverES3 = true;
    options.fDisableGpuYUVConversion = true;
    options.fReduceOpsTaskSplitting = GrContextOptions::Enable::kNo;
    options.fReducedShaderVariations = false;

    skContext = GrDirectContext::MakeGL(interface, options);

    if (!skContext.get())
    {
        qFatal() << "Failed to create Skia context.";
        return false;
    }

    skColorSpace = SkColorSpace::MakeSRGB();
    return true;
}

bool Shelf::initConfig() noexcept
{
    typeface = SkTypeface::MakeFromName(DEFAULT_FONT_FAMILY, SkFontStyle::Normal());//SkFontStyle(SkFontStyle::kSemiBold_Weight, SkFontStyle::kSemiBold_Weight, SkFontStyle::kUpright_Slant));
    tooltipFont.setTypeface(typeface);
    tooltipFont.setSize(TOOLTIP_FONT_SIZE);
    tooltipBlurMaskFilter = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, TOOLTIP_BLUR_RADIUS / 3.f);

    QIcon::setThemeName("WhiteSur");
    QIcon::setFallbackThemeName("Paper");

    defaultIcon = Utils::multiSizeImageFromPixmap(QIcon::fromTheme("terminal").pixmap(ICON_SIZE));
    trashFullIcon = Utils::multiSizeImageFromPixmap(QIcon::fromTheme("user-trash-full").pixmap(ICON_SIZE));
    trashEmptyIcon = Utils::multiSizeImageFromPixmap(QIcon::fromTheme("user-trash").pixmap(ICON_SIZE));

    separator.reset(new SeparatorController());
    trash.reset(new TrashController());

    appsDirs.reserve(16); // XDG_DATA_DIRS/applications

    for (const QString &appDir : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation))
        appsDirs.emplace_back(appDir);

    return true;
}


void Shelf::initDocks() noexcept
{
    wl_registry_destroy(wlRegistry);
    wlRegistry = wl_display_get_registry(wlDisplay);
    wl_registry_add_listener(wlRegistry, &wlRegistryListener, this);
    wl_display_roundtrip(wlDisplay);
}
