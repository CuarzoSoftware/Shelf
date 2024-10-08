#ifndef TRASHCONTROLLER_H
#define TRASHCONTROLLER_H

#include <controllers/ItemController.h>

class TrashController final : public ItemController
{
public:
    TrashController() noexcept;
    void drawItem(SkCanvas *c, Item &item) noexcept;
    void pointerEnterEvent() noexcept override;
};

#endif // TRASHCONTROLLER_H
