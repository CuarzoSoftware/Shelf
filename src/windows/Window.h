#ifndef WINDOW_H
#define WINDOW_H

#include <interfaces/Renderable.h>
#include <interfaces/PointerEventsListener.h>
#include <core/Object.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <include/core/SkSize.h>
#include <include/core/SkSurface.h>

class Window : public Object, public Renderable, public PointerEventsListener
{
public:

    enum class Type
    {
        Dock,
        Tooltip
    };

    Window(Type type) noexcept;
    ~Window() noexcept;

    Type type() const noexcept { return m_type; }

    void setMapped(bool mapped) noexcept;
    bool mapped() const noexcept { return m_mapped; }
    void commit() noexcept;

    void resize(const SkSize &size) noexcept;
    const SkSize &size() const noexcept { return m_size; }
    const SkISize &bufferSize() const noexcept { return m_bufferSize; }
    float scale() const noexcept { return m_scale; }

    void update(bool repaint = true) noexcept;

    bool hasPointerFocus() const noexcept;

    virtual void beforePaintEvent() noexcept = 0;
    virtual void afterPaintEvent() noexcept {}
    virtual void scaleChanged() noexcept { update(); }
    virtual void sizeChanged() noexcept {}
    virtual void onCallback(bool repainted) noexcept {}

    wl_surface *wlSurface() const noexcept { return m_wlSurface; }

    struct Listener
    {
        static void preferred_buffer_scale(void *data, wl_surface *wlSurface, int32_t factor);
        static void wl_callback_done(void *data, wl_callback *wlCallback, uint32_t callback_data);
    };

private:
    void updateSkSurface() noexcept;
    void emitPaintEvent() noexcept;
    SkSize m_size;
    SkISize m_bufferSize;
    bool m_pendingBufferResize { false };
    bool m_pendingRepaint { false };
    bool m_mapped { false };
    float m_scale { 1.f };
    sk_sp<SkSurface> m_skSurface;
    wl_callback *m_wlCallbackRepaint { nullptr };
    wl_callback *m_wlCallbackAnimate { nullptr };
    EGLSurface m_eglSurface { EGL_NO_SURFACE };
    wl_surface *m_wlSurface { nullptr };
    wl_egl_window *m_wlEGLWindow { nullptr };
    Type m_type;
};

#endif // WINDOW_H
