#include <windows/Window.h>
#include <core/Shelf.h>
#include <input/Seat.h>

static const wl_pointer_listener wlPointerListener
{
    .enter = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *wlSurface, wl_fixed_t surfaceX, wl_fixed_t surfaceY)
    {
        Shelf::get().focusedWindow = static_cast<Window*>(wl_surface_get_user_data(wlSurface));
        Shelf::get().pointerPos.fX = wl_fixed_to_double(surfaceX);
        Shelf::get().pointerPos.fY = wl_fixed_to_double(surfaceY);
        Shelf::get().focusedWindow->pointerEnterEvent();
    },
    .leave = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *wlSurface)
    {
        Window *window { static_cast<Window*>(wl_surface_get_user_data(wlSurface)) };

        if (Shelf::get().focusedWindow == window)
        {
            window->pointerLeaveEvent();
            Shelf::get().focusedWindow = nullptr;
        }
    },
    .motion = [](void *data, wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surfaceX, wl_fixed_t surfaceY)
    {
        if (Shelf::get().focusedWindow)
        {
            Shelf::get().pointerPos.fX = wl_fixed_to_double(surfaceX);
            Shelf::get().pointerPos.fY = wl_fixed_to_double(surfaceY);
            Shelf::get().focusedWindow->pointerMotionEvent();
        }
    },
    .button = [](void *data, wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
    {

    },
    .axis = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
    {

    }
};

static const wl_seat_listener wlSeatListener
{
    .capabilities = [](void *data, wl_seat *wl_seat, uint32_t capabilities)
    {
        auto &seat { *static_cast<Seat*>(data) };

        if (capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if (!seat.wlPointer)
            {
                seat.wlPointer = wl_seat_get_pointer(wl_seat);
                wl_pointer_add_listener(seat.wlPointer, &wlPointerListener, &seat);
            }
        }
        else
        {
            if (seat.wlPointer)
            {
                wl_pointer_destroy(seat.wlPointer);
                seat.wlPointer = nullptr;
            }
        }
    }
};

Seat::Seat(wl_seat *seat, uint32_t name) noexcept : wlSeat(seat), name(name)
{
    Shelf::get().seats.emplace_back(this);
    shelfLink = std::prev(Shelf::get().seats.end());
    wl_seat_add_listener(wlSeat, &wlSeatListener, this);
}

Seat::~Seat() noexcept
{
    Shelf::get().seats.erase(shelfLink);

    if (wlPointer)
    {
        wl_pointer_destroy(wlPointer);
        wlPointer = nullptr;
    }
}
