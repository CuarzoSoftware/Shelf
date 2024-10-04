#include <include/core/SkCanvas.h>
#include <include/core/SkMaskFilter.h>
#include <protocols/wlr-layer-shell-unstable-v1.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <Shelf.h>
#include <Dock.h>
#include <QDebug>
#include <QtMath>

static const zwlr_layer_surface_v1_listener wlLayerSurfaceListener
{
.configure = [](void *data, zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial, uint32_t width, uint32_t height)
{
    auto &dock { *static_cast<Dock*>(data) };

    dock.size.fWidth = width;
    dock.bufferSize.fWidth = dock.size.width() * dock.scale;
    dock.bufferSize.fHeight = dock.size.height() * dock.scale;

    if (dock.wlEGLWindow)
    {
        wl_egl_window_resize(dock.wlEGLWindow, dock.bufferSize.fWidth, dock.bufferSize.fHeight, 0, 0);
        dock.updateSkSurface();
        dock.render();
    }
    else
    {
        dock.wlEGLWindow = wl_egl_window_create(dock.wlSurface, dock.bufferSize.fWidth, dock.bufferSize.fHeight);
        dock.eglSurface = eglCreateWindowSurface(Shelf::get().eglDisplay, Shelf::get().eglConfig, (EGLNativeWindowType)dock.wlEGLWindow, NULL);

        if (dock.eglSurface == EGL_NO_SURFACE)
        {
            qFatal() << "Failed to create EGL window surface.";
            exit(EXIT_FAILURE);
        }

        dock.updateSkSurface();
        dock.render();
    }
},

.closed = [](void *data, zwlr_layer_surface_v1 *zwlr_layer_surface_v1)
{

}
};

static const wl_surface_listener wlSurfaceListener
{
.enter = [](void *data, wl_surface *wlSurface, wl_output *wlOutput)
{

},

.leave = [](void *data, wl_surface *wl_surface, wl_output *output)
{

},

.preferred_buffer_scale = [](void *data, wl_surface *wlSurface, int32_t factor)
{
    auto &dock { *static_cast<Dock*>(data) };
    dock.scale = factor;
    dock.bufferSize.fWidth = dock.size.width() * dock.scale;
    dock.bufferSize.fHeight = dock.size.height() * dock.scale;
    wl_surface_set_buffer_scale(wlSurface, factor);

    if (dock.wlEGLWindow)
    {
        wl_egl_window_resize(dock.wlEGLWindow, dock.bufferSize.fWidth, dock.bufferSize.fHeight, 0, 0);
        dock.updateSkSurface();
        dock.render();
    }
},

.preferred_buffer_transform = [](void *data, struct wl_surface *wl_surface, uint32_t transform)
{

}
};

static const wl_callback_listener wlCallbackListener
{
.done = [](void *data, wl_callback *wlCallback, uint32_t callback_data)
{
    wl_callback_destroy(wlCallback);
    auto &dock { *static_cast<Dock*>(data) };
    dock.wlCallback = nullptr;
    dock.render();
}
};

Dock::Dock(wl_output *wlOutput, uint32_t name) noexcept :
    wlOutput(wlOutput),
    name(name)
{
    wl_output_set_user_data(wlOutput, this);
    Shelf::get().docks.emplace_back(this);
    shelfLink = std::prev(Shelf::get().docks.end());

    wlSurface = wl_compositor_create_surface(Shelf::get().wlCompositor);
    wl_surface_add_listener(wlSurface, &wlSurfaceListener, this);

    wlLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
        Shelf::get().wlLayerShell,
        wlSurface,
        wlOutput,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        "cuarzo-shelf");
    zwlr_layer_surface_v1_set_anchor(
        wlLayerSurface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_size(
        wlLayerSurface,
        0, size.height());
    zwlr_layer_surface_v1_set_exclusive_zone(
        wlLayerSurface,
        -1);

    /* TMP */
    wl_surface_set_input_region(wlSurface, Shelf::get().wlEmptyRegion);
    wl_surface_commit(wlSurface);

    zwlr_layer_surface_v1_add_listener(wlLayerSurface, &wlLayerSurfaceListener, this);

    qInfo() << "New Dock!";
}

Dock::~Dock() noexcept
{
    wl_output_destroy(wlOutput);
    Shelf::get().docks.erase(shelfLink);
    qInfo() << "Dock destroyed!";
}

void Dock::updateSkSurface() noexcept
{
    static constexpr GrGLFramebufferInfo fbInfo
    {
        .fFBOID = 0,
        .fFormat = GL_RGBA8_OES
    };

    const GrBackendRenderTarget target(bufferSize.width(), bufferSize.height(), 0, 0, fbInfo);
    SkSurfaceProps skSurfaceProps(0, kUnknown_SkPixelGeometry);

    skSurface = SkSurface::MakeFromBackendRenderTarget(
        Shelf::get().skContext.get(),
        target,
        GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
        SkColorType::kRGBA_8888_SkColorType,
        Shelf::get().skColorSpace,
        &skSurfaceProps);

    if (!skSurface.get())
    {
        qFatal() << "Failed to create Skia surface.";
        exit(EXIT_FAILURE);
    }
}

void Dock::render() noexcept
{
    if (!wlEGLWindow || wlCallback) return;

    wlCallback = wl_surface_frame(wlSurface);
    wl_callback_add_listener(wlCallback, &wlCallbackListener, this);
    eglMakeCurrent(Shelf::get().eglDisplay, eglSurface, eglSurface, Shelf::get().eglContext);
    eglSwapInterval(Shelf::get().eglDisplay, 0);
    SkCanvas *c { skSurface->getCanvas() };
    c->save();
    c->clear(SK_ColorTRANSPARENT);
    c->scale(scale, scale);

    static float phase { 0.0 };
    phase += 0.001;

    const SkRect rect { SkRect::MakeXYWH(20, 20, size.width() - 40, 96) };

    SkPaint p;
    SkScalar blurRadius = 10.0f;
    sk_sp<SkMaskFilter> blurFilter = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blurRadius);
    p.setMaskFilter(blurFilter);
    p.setBlendMode(SkBlendMode::kSrc);

    p.setColor(SkColorSetARGB(96, 0, 0, 0));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(true);
    c->drawRoundRect(SkRect::MakeXYWH(20, 25, size.width() - 40, 96), 20, 20, p);

    p.setMaskFilter(nullptr);
    p.setColor(SkColorSetARGB(250, 255, 255, 255));
    p.setStyle(SkPaint::kFill_Style);
    p.setStrokeWidth(0.f);
    p.setAntiAlias(true);
    c->drawRoundRect(rect, 20, 20, p);

    p.setBlendMode(SkBlendMode::kSrcOver);
    p.setColor(SkColorSetARGB(96, 0, 0, 0));
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(0.5f);
    c->drawRoundRect(rect, 20, 20, p);

    p.setStyle(SkPaint::kFill_Style);
    p.setColor(SkColorSetARGB(96, 255, 0, 0));
    c->drawCircle(size.width()/2.f - qSin(phase)* (size.width()/2), 40, 10 , p);

    c->flush();
    c->restore();
    eglSwapBuffers(Shelf::get().eglDisplay, eglSurface);
}
