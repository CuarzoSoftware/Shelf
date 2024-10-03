#ifndef THUMBNAILCONTROLLER_H
#define THUMBNAILCONTROLLER_H

#include <controller/ItemController.h>

class ToplevelHandle;

/* For minimized windows */
class ThumbnailController final : public ItemController
{
public:
    ThumbnailController(ToplevelHandle &toplevelHandle) noexcept;
    ToplevelHandle &toplevelHandle;

    void onItemClick(DockItem *item) noexcept;
};

#endif // THUMBNAILCONTROLLER_H
