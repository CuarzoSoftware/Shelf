#ifndef OBJECT_H
#define OBJECT_H

#include <Shelf.h>

class Object
{
public:
    Shelf &shelf() const noexcept { return Shelf::get(); }
    wl_compositor *wlCompositor() const noexcept { return Shelf::get().wlCompositor; }
    EGLDisplay eglDisplay() const noexcept { return Shelf::get().eglDisplay; }
    EGLContext eglContext() const noexcept { return Shelf::get().eglContext; }
};

#endif // OBJECT_H
