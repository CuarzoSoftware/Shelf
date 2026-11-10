#ifndef DOCKBACKGROUND_H
#define DOCKBACKGROUND_H

#include <Core/Types.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>

/**
 * @brief The dock fill: a translucent tint over the compositor's background blur when it is
 *        supported (vibrancy enabled), or an opaque fallback color otherwise.
 */
class DockBackground final : public MVibrancyView
{
public:
    DockBackground(AKNode *parent = nullptr) noexcept;

protected:
    bool event(const CZEvent &event) noexcept override;
    void updateColor() noexcept;
};

#endif // DOCKBACKGROUND_H
