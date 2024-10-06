#include <core/Shelf.h>
#include <core/Object.h>

Shelf &Object::shelf() const noexcept { return Shelf::get(); }
wl_compositor *Object::wlCompositor() const noexcept { return Shelf::get().wlCompositor; }
EGLDisplay Object::eglDisplay() const noexcept { return Shelf::get().eglDisplay; }
EGLContext Object::eglContext() const noexcept { return Shelf::get().eglContext; }
