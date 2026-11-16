#include <Nodes/DockIcon.h>
#include <Core/Shelf.h>
#include <cmath>

DockIcon::DockIcon(std::shared_ptr<RImage> image, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionColumn, false, parent)
{
    SkRegion empty;
    setInputRegion(&empty);
    frame.setInputRegion(&empty);

    // The pre-rendered image covers the icon plus the shadow margin around it (see setShadowMargin())
    frame.setSizeMode(AKImageFrame::SizeMode::Fill);
    frame.layout().setPositionType(YGPositionTypeAbsolute);
    setShadowMargin(MarginFor(DOCK_TILE_SIZE_DEFAULT));
    setImage(image);
}

void DockIcon::setImage(std::shared_ptr<RImage> image) noexcept
{
    if (m_image == image && (image || !frame.image()))
        return;

    m_image = image;

    // Rendered for the size it's drawn at on each screen: the frame minus the margin on each side is
    // the icon (in pixels, both whole numbers)
    if (image)
        frame.setImageProvider([this, image](Int32 pixels, Int32 scale){
            return GetShelf()->assets.shadowedIcon(image, pixels, pixels - 2 * m_margin * scale, scale);
        });
    else
        frame.setImage(nullptr);
}

Int32 DockIcon::MarginFor(SkScalar size) noexcept
{
    return Int32(std::ceil(std::max(0.f, size) * Assets::ShadowMargin()));
}

void DockIcon::setShadowMargin(Int32 margin) noexcept
{
    margin = std::max(0, margin);

    if (margin == m_margin)
        return;

    m_margin = margin;

    // Stretched between the insets: the node's size plus the margin on each side, laid out in the
    // same pass as the node (whole units: the frame's edges, and the icon in it, on the pixel grid)
    for (YGEdge edge : { YGEdgeLeft, YGEdgeTop, YGEdgeRight, YGEdgeBottom })
        frame.layout().setPosition(edge, -margin);
}
