#ifndef LAUNCHBOUNCE_H
#define LAUNCHBOUNCE_H

#include <Core/Types.h>
#include <CZ/Marco/Roles/MSubsurface.h>
#include <CZ/AK/Nodes/AKImageFrame.h>
#include <CZ/Core/CZLinearAnimation.h>
#include <CZ/Core/CZWeak.h>
#include <chrono>
#include <functional>

/**
 * @brief The macOS launch feedback: the app's icon bouncing over its dock item.
 *
 * A subsurface of the dock showing a copy of the icon, moved every frame (so only a position
 * changes, nothing is redrawn). The item's own icon is hidden meanwhile (its space is kept).
 *
 * It bounces until stop() (the app showed a window) or a timeout, always finishing the current
 * bounce so the icon lands, then onFinished is invoked (from a later event loop iteration, so the
 * owner may destroy it there).
 */
class LaunchBounce final : public MSubsurface
{
public:
    LaunchBounce(Dock &dock, std::shared_ptr<RImage> icon) noexcept;
    ~LaunchBounce();

    /// The dock item it bounces over (its icon is hidden while bouncing). May change when the dock
    /// rebuilds its items.
    void setItem(DockItem *item) noexcept;

    /// Lands on the current bounce and finishes.
    void stop() noexcept;

    std::function<void()> onFinished;

private:
    void updatePosition() noexcept;
    void finish() noexcept;

    Dock &m_dock;
    AKImageFrame m_icon { this };
    CZWeak<DockItem> m_item;
    CZLinearAnimation m_anim;
    SkScalar m_height { 0.f };     // current offset above the item
    bool m_stopping { false };
    bool m_finished { false };
    std::chrono::steady_clock::time_point m_start;
};

#endif // LAUNCHBOUNCE_H
