#include <Nodes/DockBackground.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>

DockBackground::DockBackground(AKNode *parent) noexcept : MVibrancyView(parent)
{
    SkRegion empty;
    setInputRegion(&empty);
    setDisabledColor(DOCK_FALLBACK_COLOR);
    updateColor();
}

bool DockBackground::event(const CZEvent &event) noexcept
{
    const bool ret { MVibrancyView::event(event) };

    if (event.type() == AKVibrancyEvent::Type::Vibrancy)
        updateColor();

    return ret;
}

void DockBackground::updateColor() noexcept
{
    // MVibrancyView makes the fill fully transparent while vibrancy is enabled; keep a tint.
    if (vibrancyEnabled())
        setColor(DOCK_TINT_COLOR);
}
