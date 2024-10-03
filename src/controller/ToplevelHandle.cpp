#include <controller/AppController.h>
#include <controller/ThumbnailController.h>
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
    .state          = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle, wl_array *state)
    {
        auto &handle { *static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle)) };
        handle.pendingState = 0;

        for (size_t i = 0; i < state->size; i++)
            handle.pendingState |= 1 << static_cast<zwlr_foreign_toplevel_handle_v1_state*>(state->data)[i];
    },
    .done           = [](void *data, zwlr_foreign_toplevel_handle_v1 *whandle)
    {
        auto &handle { *static_cast<ToplevelHandle*>(zwlr_foreign_toplevel_handle_v1_get_user_data(whandle)) };

        handle.currentTitle = handle.pendingTitle;

        if (handle.currentAppId != handle.pendingAppId)
        {
            if (handle.appController)
                handle.appController->removeToplevelHandle(&handle);

            handle.currentAppId = handle.pendingAppId;

            qInfo() << "New app id" << handle.currentAppId << handle.currentTitle;

            AppController *newController { nullptr };
            QString test;

            for (AppController *c : Shelf::get().pinnedApps)
            {
                if (c->id() == handle.currentAppId ||
                    c->idName() == handle.currentAppId.toLower() ||
                    c->idName() == handle.currentTitle.toLower() ||
                    c->name().toLower() == handle.currentTitle.toLower())
                {
                    newController = c;
                    break;
                }


                int64_t i = c->iconName().lastIndexOf(".");

                if (i != -1 && c->iconName().sliced(i + 1).toLower() == handle.currentAppId.toLower())
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

            if (!newController)
            {
                QString desktopFilePath { Shelf::desktopFileFromAppId(handle.currentAppId) };
                KDesktopFile df(desktopFilePath);
                QString iconName = df.readIcon();
                QString name = df.readName();

                if (iconName.isNull())
                {
                    iconName = handle.currentAppId;
                    name = handle.currentAppId;
                }


                for (AppController *c : Shelf::get().pinnedApps)
                {
                    if (c->iconName() == iconName || iconName == c->id() || iconName.toLower() == c->idName())
                    {
                        newController = c;
                        break;
                    }
                }

                if (!newController)
                {
                    newController = new AppController(iconName, name, handle.currentAppId, desktopFilePath, false, true);
                    newController->addToplevelHandle(&handle);
                    goto skipAppId;
                }
            }

            if (newController)
            {
                newController->addToplevelHandle(&handle);
                goto skipAppId;
            }
        }

        skipAppId:

        handle.currentState = handle.pendingState;

        bool minimized = (handle.currentState & (1 << ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)) != 0;

        qInfo() << handle.currentAppId << "minimized" << minimized;

        if (minimized)
        {
            if (handle.appController && !handle.thumbnailController)
                handle.thumbnailController = new ThumbnailController(handle);
        }
        else
        {
            if (handle.thumbnailController)
            {
                delete handle.thumbnailController;
                handle.thumbnailController = nullptr;
            }
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

    if (thumbnailController)
    {
        delete thumbnailController;
        thumbnailController = nullptr;
    }

    Shelf::get().toplevelHandles.erase(shelfLink);
    zwlr_foreign_toplevel_handle_v1_destroy(handle);
}

void ToplevelHandle::sendActivate() noexcept
{
    zwlr_foreign_toplevel_handle_v1_activate(handle, Shelf::get().seat);
}

void ToplevelHandle::sendMinimized(bool minimized) noexcept
{
    if (minimized)
        zwlr_foreign_toplevel_handle_v1_set_minimized(handle);
    else
        zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
}
