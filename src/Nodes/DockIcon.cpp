#include <Nodes/DockIcon.h>
#include <Core/Shelf.h>

DockIcon::DockIcon(std::shared_ptr<RImage> image, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionColumn, false, parent)
{
    SkRegion empty;
    setInputRegion(&empty);
    frame.setInputRegion(&empty);

    // The pre-rendered image covers the icon plus the shadow margin around it (as a fraction of the
    // icon size, see Assets::shadowedIcon()).
    const float margin { 100.f * Assets::ShadowMargin() };
    frame.setSizeMode(AKImageFrame::SizeMode::Fill);
    frame.layout().setPositionType(YGPositionTypeAbsolute);
    frame.layout().positionPercent(YGEdgeLeft, -margin);
    frame.layout().positionPercent(YGEdgeTop, -margin);
    frame.layout().setWidthPercent(100.f + 2.f * margin);
    frame.layout().setHeightPercent(100.f + 2.f * margin);

    setImage(image);
}

void DockIcon::setImage(std::shared_ptr<RImage> image) noexcept
{
    if (m_image == image && (image || !frame.image()))
        return;

    m_image = image;
    frame.setImage(GetShelf()->assets.shadowedIcon(image));
}
