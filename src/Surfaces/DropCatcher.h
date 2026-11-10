#ifndef DROPCATCHER_H
#define DROPCATCHER_H

#include <Core/Types.h>
#include <CZ/Marco/Roles/MLayerSurface.h>

class Dock;

/**
 * @brief A transparent overlay covering a screen while an app is dragged out of the dock, so the
 *        drop is taken wherever the pointer is released (the app keeps the slot it was dragged to,
 *        like on macOS) instead of being rejected.
 *
 * Forwards the drag (in screen coordinates) to its dock, which translates the positions over the
 * dock to its own coordinates to keep reordering.
 */
class DropCatcher final : public MLayerSurface
{
public:
    DropCatcher(Dock &dock, MScreen &screen) noexcept;

    /// The screen's logical size (the surface covers it).
    SkISize screenSize() const noexcept { return suggestedSize(); }

private:
    Dock &m_dock;
};

#endif // DROPCATCHER_H
