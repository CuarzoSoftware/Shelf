#include <controller/ItemController.h>
#include <protocols/wlr-foreign-toplevel-management-unstable-v1-client.h>
#include <controller/ToplevelHandle.h>
#include <controller/Shelf.h>
#include <kdesktopfile.h>

static zwlr_foreign_toplevel_handle_v1_listener handleListener
{
    .title          = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, const char *title)
    {
        auto &handle { *static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle)) };
        handle.pendingTitle = title;
    },
    .app_id         = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, const char *appId)
    {
        auto &handle { *static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle)) };
        handle.pendingAppId = appId;
    },
    .output_enter   = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, wl_output *output) {},
    .output_leave   = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, wl_output *output) {},
    .state          = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, wl_array *state) {},
    .done           = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle)
    {
        auto &handle { *static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle)) };

        if (handle.currentAppId != handle.pendingAppId)
        {
            handle.currentAppId = handle.pendingAppId;

            /*
            QString desktopFilePath { Shelf::desktopFileFromAppId(handle.currentAppId) };
            KDesktopFile df(desktopFilePath);
            QString iconName = df.readIcon();

            if (iconName.isNull())
                iconName = handle.currentAppId;

            new ItemController(ItemController::App, false, df.readName(), handle.currentAppId, desktopFilePath,
                               Shelf::iconFromName(iconName, 512));*/

        }


        handle.currentTitle = handle.pendingTitle;
    },
    .closed         = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle)
    {
        delete static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle));
    },
    .parent         = nullptr
};

ToplevelHandle::ToplevelHandle(zwlr_foreign_toplevel_handle_v1 *whandle) noexcept :
    handle(whandle)
{
    Shelf::get().toplevelHandles.push_back(this);
    link = std::prev(Shelf::get().toplevelHandles.end());
    zwlr_foreign_toplevel_handle_v1_add_listener(whandle, &handleListener, this);
}

ToplevelHandle::~ToplevelHandle() noexcept
{
    Shelf::get().toplevelHandles.erase(link);
    zwlr_foreign_toplevel_handle_v1_destroy(handle);
}
