#ifndef DOCK_H
#define DOCK_H

#include <list>
#include <inttypes.h>
#include <EGL/egl.h>
#include <include/core/SkSize.h>
#include <include/core/SkSurface.h>
#include <windows/Window.h>
#include <chrono>

struct wl_output;
struct zwlr_layer_surface_v1;
class Item;

class Dock final : public Window
{
public:
    Dock(wl_output *wlOutput, uint32_t name) noexcept;
    ~Dock() noexcept;

    void createItems() noexcept;
    void destroyItems() noexcept;

    void updateDimensions() noexcept;
    void beforePaintEvent() noexcept override;
    void paintEvent(SkCanvas *c) noexcept override;
    void drawContainer(SkCanvas *c) noexcept;
    void drawItems(SkCanvas *c) noexcept;
    void onCallback(bool repainted) noexcept override;
    void sizeChanged() noexcept override;
    void pointerEnterEvent() noexcept override;
    void pointerLeaveEvent() noexcept override;
    void animateVisibility(float from, float to, int64_t ms) noexcept;
    void setVisibility(float v) noexcept;

    std::list<Item*> items;
    Item *separatorItem, *trashItem;

    /* Dimensions */

    SkRect rect;
    float dockScale { 1.f };
    float borderRadius;
    float paddingT, paddingL, paddingR;
    float shadowRadius, shadowOffsetY;
    float itemSize, itemSpacing;
    float dotSize, dotMarginT, dotMarginB;

    /* Wayland */

    wl_output *wlOutput;
    zwlr_layer_surface_v1 *wlLayerSurface { nullptr };
    std::list<Dock*>::iterator shelfLink;
    uint32_t name;

    /* Animation */

    int32_t y { 0 };
    bool sliding { false };
    float visibility { 0.f };
    float fromVisibility, toVisibility;
    std::chrono::time_point<std::chrono::steady_clock> slideAnimStart;
    int64_t slideAnimDurationMs;
};

#endif // DOCK_H
