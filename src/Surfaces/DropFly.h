#ifndef DROPFLY_H
#define DROPFLY_H

#include <Core/Types.h>
#include <Nodes/DockIcon.h>
#include <CZ/Marco/Roles/MSubsurface.h>
#include <CZ/Core/CZLinearAnimation.h>
#include <CZ/Core/CZWeak.h>
#include <functional>
#include <string>

/**
 * @brief An app dropped away from the dock flying from the pointer back into its slot.
 *
 * A subsurface of the dock (it may be anywhere on the screen) with a copy of the icon. The item's
 * own icon stays hidden until it lands; then onFinished is invoked (from a later event loop
 * iteration, so the owner may destroy it there).
 */
class DropFly final : public MSubsurface
{
public:
    /// @p from: the icon rect where it was dropped, in dock surface coordinates.
    DropFly(Dock &dock, std::shared_ptr<RImage> icon, const std::string &appId, const SkIRect &from) noexcept;
    ~DropFly();

    const std::string &appId() const noexcept { return m_appId; }

    /// The item it lands on (may change when the dock rebuilds its items).
    void setItem(DockItem *item) noexcept;

    std::function<void()> onFinished;

private:
    void update(Float64 t) noexcept;

    Dock &m_dock;
    DockIcon m_icon { nullptr, this };
    std::string m_appId;
    SkIRect m_from;
    CZWeak<DockItem> m_item;
    CZLinearAnimation m_anim;
    bool m_started { false };
    bool m_finished { false };

    // Started once the subsurface is on screen (its first frame callback), so the whole flight is
    // seen. Falls back to a timer in case no frame comes.
    void start() noexcept;
};

#endif // DROPFLY_H
