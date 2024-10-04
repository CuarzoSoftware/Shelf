#ifndef DOCK_H
#define DOCK_H

#include <list>
#include <inttypes.h>

struct wl_output;
struct zwlr_layer_surface_v1;

class Dock
{
public:
    Dock(wl_output *wlOutput, uint32_t name) noexcept;
    ~Dock() noexcept;

    wl_output *wlOutput;
    zwlr_layer_surface_v1 *wlLayerSurface { nullptr };
    std::list<Dock*>::iterator shelfLink;
    uint32_t name;
};

#endif // DOCK_H
