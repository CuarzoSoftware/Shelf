#include "controller/AppController.h"
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

        handle.currentTitle = handle.pendingTitle;

        if (handle.currentAppId != handle.pendingAppId)
        {
            if (handle.appController)
                handle.appController->removeToplevelHandle(&handle);

            handle.currentAppId = handle.pendingAppId;

            AppController *newController { nullptr };

            for (AppController *c : Shelf::get().pinnedApps)
            {
                if (c->id() == handle.currentAppId)
                {
                    newController = c;
                    break;
                }
            }

            if (!newController)
            {
                for (AppController *c : Shelf::get().unpinnedApps)
                {
                    if (c->id() == handle.currentAppId)
                    {
                        newController = c;
                        break;
                    }
                }
            }

            if (newController)
            {
                newController->addToplevelHandle(&handle);
                return;
            }

            QString desktopFilePath { Shelf::desktopFileFromAppId(handle.currentAppId) };
            KDesktopFile df(desktopFilePath);
            QString iconName = df.readIcon();
            QString name = df.readName();

            if (iconName.isNull())
            {
                iconName = handle.currentAppId;
                name = handle.currentAppId;
            }

            newController = new AppController(iconName, name, handle.currentAppId, desktopFilePath, false, true);
            newController->addToplevelHandle(&handle);
        }
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
    shelfLink = std::prev(Shelf::get().toplevelHandles.end());
    zwlr_foreign_toplevel_handle_v1_add_listener(whandle, &handleListener, this);
}

ToplevelHandle::~ToplevelHandle() noexcept
{
    if (appController)
        appController->removeToplevelHandle(this);

    Shelf::get().toplevelHandles.erase(shelfLink);
    zwlr_foreign_toplevel_handle_v1_destroy(handle);
}
