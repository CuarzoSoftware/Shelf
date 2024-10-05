#include <Window.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <QDebug>

void Window::Listener::preferred_buffer_scale(void *data, wl_surface *wlSurface, int32_t factor)
{
    Window &window { *static_cast<Window*>(data) };
    wl_surface_set_buffer_scale(wlSurface, factor);
    window.m_scale = factor;
    window.resize(window.m_size);
    window.scaleChanged();
}

void Window::Listener::wl_callback_done(void *data, wl_callback *wlCallback, uint32_t callback_data)
{
    wl_callback_destroy(wlCallback);
    Window &window { *static_cast<Window*>(data) };

    if (wlCallback == window.m_wlCallbackRepaint)
    {
        window.m_wlCallbackRepaint = nullptr;
        window.onCallback(true);

        if (window.m_pendingRepaint)
            window.emitPaintEvent();
    }
    else
    {
        window.m_wlCallbackAnimate = nullptr;
        window.onCallback(false);
    }
}

static const wl_surface_listener wlSurfaceListener
{
    .enter = [](void *data, wl_surface *wlSurface, wl_output *wlOutput) {},
    .leave = [](void *data, wl_surface *wlSurface, wl_output *wlOutput) {},
    .preferred_buffer_scale = &Window::Listener::preferred_buffer_scale,
    .preferred_buffer_transform = [](void *data, struct wl_surface *wl_surface, uint32_t transform) {}
};

static const wl_callback_listener wlCallbackListener
{
    .done = &Window::Listener::wl_callback_done
};

Window::Window(Type type) noexcept : m_type(type)
{
    m_wlSurface = wl_compositor_create_surface(wlCompositor());
    wl_surface_add_listener(m_wlSurface, &wlSurfaceListener, this);
}

Window::~Window() noexcept
{
    if (m_skSurface.get())
        m_skSurface.reset();

    if (m_eglSurface != EGL_NO_SURFACE)
    {
        eglDestroySurface(eglDisplay(), m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_wlEGLWindow)
    {
        wl_egl_window_destroy(m_wlEGLWindow);
        m_wlEGLWindow = nullptr;
    }

    if (m_wlCallbackRepaint)
    {
        wl_callback_destroy(m_wlCallbackRepaint);
        m_wlCallbackRepaint = nullptr;
    }

    if (m_wlCallbackAnimate)
    {
        wl_callback_destroy(m_wlCallbackAnimate);
        m_wlCallbackAnimate = nullptr;
    }

    if (m_wlSurface)
    {
        wl_surface_destroy(m_wlSurface);
        m_wlSurface = nullptr;
    }
}

void Window::setMapped(bool mapped) noexcept
{
    if (mapped == m_mapped)
        return;

    m_mapped = mapped;

    if (mapped)
    {
        if (m_wlEGLWindow)
            wl_egl_window_resize(m_wlEGLWindow, m_bufferSize.fWidth, m_bufferSize.fHeight, 0, 0);
        else
        {
            m_wlEGLWindow = wl_egl_window_create(wlSurface(), m_bufferSize.fWidth, m_bufferSize.fHeight);
            m_eglSurface = eglCreateWindowSurface(Shelf::get().eglDisplay, Shelf::get().eglConfig, (EGLNativeWindowType)m_wlEGLWindow, NULL);

            if (m_eglSurface == EGL_NO_SURFACE)
            {
                qFatal() << "Failed to create EGL window surface.";
                exit(EXIT_FAILURE);
            }
        }

        updateSkSurface();
        update();
    }
    else
    {
        wl_surface_attach(wlSurface(), NULL, 0, 0);
        commit();
    }
}

void Window::commit() noexcept
{
    wl_surface_commit(wlSurface());
}

void Window::resize(const SkSize &size) noexcept
{
    m_size = size;
    m_bufferSize.fWidth = m_size.width() * m_scale;
    m_bufferSize.fHeight = m_size.height() * m_scale;

    if (mapped())
    {
        wl_egl_window_resize(m_wlEGLWindow, m_bufferSize.fWidth, m_bufferSize.fHeight, 0, 0);
        updateSkSurface();
    }
}

void Window::update(bool repaint) noexcept
{
    if (repaint)
    {
        m_pendingRepaint = true;
        emitPaintEvent();
    }
    else if (!m_wlCallbackAnimate)
    {
        m_wlCallbackAnimate = wl_surface_frame(wlSurface());
        wl_callback_add_listener(m_wlCallbackAnimate, &wlCallbackListener, this);
        commit();
    }
}

void Window::updateSkSurface() noexcept
{
    static constexpr GrGLFramebufferInfo fbInfo
    {
        .fFBOID = 0,
        .fFormat = GL_RGBA8_OES
    };

    const GrBackendRenderTarget target(m_bufferSize.width(), m_bufferSize.height(), 0, 0, fbInfo);
    const SkSurfaceProps skSurfaceProps(0, kUnknown_SkPixelGeometry); // TODO: Use wl_output pix geo

    m_skSurface = SkSurface::MakeFromBackendRenderTarget(
        Shelf::get().skContext.get(),
        target,
        GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
        SkColorType::kRGBA_8888_SkColorType,
        Shelf::get().skColorSpace,
        &skSurfaceProps);

    if (!m_skSurface.get())
    {
        qFatal() << "Failed to create Skia surface.";
        exit(EXIT_FAILURE);
    }
}

void Window::emitPaintEvent() noexcept
{
    if (!mapped() || m_wlCallbackRepaint)
        return;

    m_wlCallbackRepaint = wl_surface_frame(wlSurface());
    wl_callback_add_listener(m_wlCallbackRepaint, &wlCallbackListener, this);
    m_pendingRepaint = false;
    eglMakeCurrent(eglDisplay(), m_eglSurface, m_eglSurface, eglContext());
    eglSwapInterval(eglDisplay(), 0);
    paintEvent(m_skSurface->getCanvas());
    eglSwapBuffers(eglDisplay(), m_eglSurface);
    eglMakeCurrent(eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext());
}
