#ifndef SEPARATORCONTROLLER_H
#define SEPARATORCONTROLLER_H

#include <controllers/ItemController.h>
#include <include/core/SkPoint.h>

class SeparatorController final : public ItemController
{
public:
    SeparatorController() noexcept;
    void drawItem(SkCanvas *c, Item &item) noexcept override;
    void pointerEnterEvent() noexcept override;
    void pointerLeaveEvent() noexcept override;
    void pointerMotionEvent() noexcept override;
    void pointerButtonEvent() noexcept override;

    SkPoint dragInitPos;
    float dragInitDockScale;
};

#endif // SEPARATORCONTROLLER_H
