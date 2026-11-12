#ifndef DOCKICON_H
#define DOCKICON_H

#include <Core/Types.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKImageFrame.h>
#include <memory>

/**
 * @brief A dock tile's icon with a faint drop shadow following its shape.
 *
 * The icon and its shadow are pre-rendered together into one image (Assets::shadowedIcon(), cached
 * per icon and size, rendered for the size it's drawn at on each screen) shown by a plain image frame: no live shadow effect to recompute when repainting. The
 * frame extends past the node by the shadow's margin, as a percentage of the node, so it follows
 * the tile size (resizing the dock, items growing/shrinking).
 */
class DockIcon final : public AKContainer
{
public:
    DockIcon(std::shared_ptr<RImage> image = nullptr, AKNode *parent = nullptr) noexcept;

    void setImage(std::shared_ptr<RImage> image) noexcept;

    /// The icon itself (without the shadow).
    std::shared_ptr<RImage> image() const noexcept { return m_image; }

    /// Opacity of the icon and its shadow.
    void setIconOpacity(SkScalar opacity) noexcept { frame.setOpacity(opacity); }

    /// Multiplies the colors (e.g. darkening a pressed item).
    void setColorFactor(const SkColor4f &factor) noexcept { frame.setColorFactor(factor); }

    AKImageFrame frame { this };

private:
    std::shared_ptr<RImage> m_image;
};

#endif // DOCKICON_H
