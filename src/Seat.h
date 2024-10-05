#ifndef SEAT_H
#define SEAT_H

#include <wayland-client.h>
#include <list>

class Seat
{
public:
    Seat(wl_seat *seat, uint32_t name) noexcept;
    ~Seat() noexcept;

    wl_seat *wlSeat { nullptr };
    wl_pointer *wlPointer { nullptr };

    uint32_t name;

    std::list<Seat*>::iterator shelfLink;
};

#endif // SEAT_H
