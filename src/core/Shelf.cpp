#include <include/gpu/gl/GrGLAssembleInterface.h>
#include <QStandardPaths>
#include <QIcon>
#include <QDebug>

#include <core/Utils.h>
#include <core/Shelf.h>
#include <windows/Dock.h>
#include <input/Seat.h>

static const wl_registry_listener wlRegistryListener {
.global = [](void *data, wl_registry *wlRegistry, uint32_t name, const char *interface, uint32_t version)
{
    auto &shelf { *static_cast<Shelf*>(data) };

    if (!shelf.wlCompositor && version >= 6 && strcmp(interface, wl_compositor_interface.name) == 0)
    {
        shelf.wlCompositor = (wl_compositor*)wl_registry_bind(wlRegistry, name, &wl_compositor_interface, version);
        return;
    }

    if (!shelf.wlLayerShell && strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
        shelf.wlLayerShell = (zwlr_layer_shell_v1*)wl_registry_bind(wlRegistry, name, &zwlr_layer_shell_v1_interface, 1);
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

int Shelf::init() noexcept
{
    if (!initWayland())
        return EXIT_FAILURE;

    if (!initEGL())
        return EXIT_FAILURE;

    if (!initSkia())
        return EXIT_FAILURE;

    initConfig();

    running = true;

    /* Get wl_outputs */
    initDocks();

    while (running)
    {
        if (wl_display_dispatch(wlDisplay) == -1)
            return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
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

    if (!wlLayerShell)
    {
        qFatal() << "Could not get zwlr_layer_shell_v1 version 1.";
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

void Shelf::initDocks() noexcept
{
    wl_registry_destroy(wlRegistry);
    wlRegistry = wl_display_get_registry(wlDisplay);
    wl_registry_add_listener(wlRegistry, &wlRegistryListener, this);
    wl_display_roundtrip(wlDisplay);
}

bool Shelf::initConfig() noexcept
{
    appsDirs.reserve(16); // XDG_DATA_DIRS/applications

    for (const QString &appDir : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation))
        appsDirs.emplace_back(appDir);

    //QIcon::setThemeName("WhiteSur");
    //QIcon::setFallbackThemeName("Paper");
    defaultIcon = Utils::skImageFromPixmap(Utils::iconApplyShadow(QIcon::fromTheme("firefox").pixmap(96), 96));
    return true;
}
