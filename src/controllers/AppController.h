#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <controllers/ItemController.h>

class AppController final : public ItemController
{
public:
    AppController() noexcept;

    void drawItem(SkCanvas *c, Item &item) noexcept override;
};

#endif // APPCONTROLLER_H
