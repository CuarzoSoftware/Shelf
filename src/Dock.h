#ifndef DOCK_H
#define DOCK_H

#include <Marco/roles/MLayerSurface.h>
#include <AK/nodes/AKThreeImagePatch.h>
#include <AK/AKAnimation.h>

using namespace AK;

class Dock : public MLayerSurface
{
public:
    Dock(MScreen *screen) noexcept;

    SkScalar screenWidth() const noexcept;

    void pointerEnterEvent(const AKPointerEnterEvent &e) override;
    void pointerLeaveEvent(const AKPointerLeaveEvent &e) override;

    void showDock() noexcept;
    void hideDock() noexcept;

    AKAnimation slideUpAnim, slideDownAnim;
    void slideUpAnimUpdate() noexcept;
    void slideUpAnimFinish() noexcept;
    void slideDownAnimUpdate() noexcept;
    void slideDownAnimFinish() noexcept;

    AKThreeImagePatch background { AKThreeImagePatch::Horizontal, this };
    bool hover { false };

protected:
    void layoutEvent(const AKLayoutEvent &e) override;

private:
    void setupBackground() noexcept;
    void setupAnimations() noexcept;
};

#endif // DOCK_H
