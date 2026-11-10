#include <Surfaces/DropCatcher.h>
#include <Surfaces/Dock.h>

DropCatcher::DropCatcher(Dock &dock, MScreen &screen) noexcept :
    MLayerSurface(),
    m_dock(dock)
{
    setScreen(&screen);
    setScope("cz-shelf-drop");
    setLayer(Layer::Overlay);
    setAnchor(CZEdgeLeft | CZEdgeTop | CZEdgeRight | CZEdgeBottom);
    setExclusiveZone(-1);
    setKeyboardInteractivity(KeyboardInteractivity::None);
    requestAvailableWidth();
    requestAvailableHeight();
    setColor(CZAdaptiveColor(0x00000000)); // Invisible: only takes the drop

    onDNDEnter.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){ m_dock.catcherDragOver(offer, pos); });
    onDNDMotion.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){ m_dock.catcherDragOver(offer, pos); });
    onDNDDrop.subscribe(this, [this](MDNDOffer &offer, const SkPoint &pos){ m_dock.catcherDropped(offer, pos); });
}
