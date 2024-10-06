#ifndef OBJECT_H
#define OBJECT_H

#include <EGL/egl.h>

class Shelf;
struct wl_compositor;

class Object
{
public:
    Shelf &shelf() const noexcept;
    wl_compositor *wlCompositor() const noexcept;
    EGLDisplay eglDisplay() const noexcept;
    EGLContext eglContext() const noexcept;
};

#endif // OBJECT_H
