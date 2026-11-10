#ifndef DOCKICON_H
#define DOCKICON_H

#include <Core/Types.h>
#include <CZ/AK/Nodes/AKBakeable.h>
#include <CZ/AK/Effects/AKBackgroundImageShadowEffect.h>
#include <memory>

/**
 * @brief A dock tile's icon, scaled to fit the node, with a faint drop shadow following its shape.
 *
 * Baked (rather than an AKImageFrame) because the shadow effect is generated from the baked image.
 * Use setIconOpacity() to fade it: it fades the shadow too (the effect is a separate node).
 */
class DockIcon final : public AKBakeable
{
public:
    DockIcon(std::shared_ptr<RImage> image = nullptr, AKNode *parent = nullptr) noexcept;

    void setImage(std::shared_ptr<RImage> image) noexcept;
    std::shared_ptr<RImage> image() const noexcept { return m_image; }

    /// Opacity of the icon and its shadow.
    void setIconOpacity(SkScalar opacity) noexcept;

    AKBackgroundImageShadowEffect shadow { DOCK_ICON_SHADOW_RADIUS, { 0, DOCK_ICON_SHADOW_OFFSET_Y }, DOCK_ICON_SHADOW_COLOR, this };

protected:
    void bakeEvent(const AKBakeEvent &event) override;

private:
    std::shared_ptr<RImage> m_image;
};

#endif // DOCKICON_H
