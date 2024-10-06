#ifndef SEPARATORCONTROLLER_H
#define SEPARATORCONTROLLER_H

#include <controllers/ItemController.h>

class SeparatorController final : public ItemController
{
public:
    SeparatorController() noexcept;
    void drawItem(SkCanvas *c, Item &item) noexcept override;
};

#endif // SEPARATORCONTROLLER_H
