#include <controller/ThumbnailController.h>
#include <controller/AppController.h>
#include <controller/ToplevelHandle.h>

ThumbnailController::ThumbnailController(ToplevelHandle &toplevelHandle) noexcept :
    ItemController(
        Thumbnail,
        toplevelHandle.currentTitle,
        toplevelHandle.appController->icon(),
        false,
        ""),
    toplevelHandle(toplevelHandle)
{
    startScaleAnim(0.0, 1.0);
}

void ThumbnailController::onItemClick(DockItem *item) noexcept
{
    toplevelHandle.sendMinimized(false);
    toplevelHandle.sendActivate();
}
