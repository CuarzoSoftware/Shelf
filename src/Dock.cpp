#include <wayland-client.h>
#include <Shelf.h>
#include <Dock.h>
#include <QDebug>

Dock::Dock(wl_output *wlOutput, uint32_t name) noexcept :
    wlOutput(wlOutput),
    name(name)
{
    wl_output_set_user_data(wlOutput, this);
    Shelf::get().docks.emplace_back(this);
    shelfLink = std::prev(Shelf::get().docks.end());
    qInfo() << "New Dock!";
}

Dock::~Dock() noexcept
{
    wl_output_destroy(wlOutput);
    Shelf::get().docks.erase(shelfLink);
    qInfo() << "Dock destroyed!";
}
