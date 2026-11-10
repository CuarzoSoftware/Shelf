#ifndef DOCKDIVIDER_H
#define DOCKDIVIDER_H

#include <Core/Types.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKThreePatch.h>

/**
 * @brief The vertical line between the pinned apps and the trash / minimized windows.
 *
 * Like on macOS, dragging it resizes the dock: up makes it bigger, down smaller. The whole slot
 * (not only the 1-unit line) is draggable and shows a vertical resize cursor.
 */
class DockDivider final : public AKContainer
{
public:
    DockDivider(Dock &dock, AKNode *parent = nullptr) noexcept;

    /// Applies the slot width and line length for @p metrics.
    void setMetrics(const DockMetrics &metrics) noexcept;

    bool dragging() const noexcept { return m_dragging; }

protected:
    void pointerButtonEvent(const CZPointerButtonEvent &event) override;
    void pointerMoveEvent(const CZPointerMoveEvent &event) override;

private:
    class Line final : public AKThreePatch
    {
    public:
        Line(AKNode *parent) noexcept;
    protected:
        void onSceneBegin() override;
    };

    // Pointer distance to the bottom of the surface. The dock is anchored to the bottom, so this is
    // stable while the surface grows/shrinks during the drag (unlike surface-local y).
    Float32 pointerFromBottom() const noexcept;
    void endDrag() noexcept;

    Dock &m_dock;
    Line m_line { this };
    bool m_dragging { false };
    Float32 m_dragStartFromBottom { 0.f };
    Int32 m_dragStartTile { 0 };
    bool m_resized { false }; // The current drag changed the size
};

#endif // DOCKDIVIDER_H
