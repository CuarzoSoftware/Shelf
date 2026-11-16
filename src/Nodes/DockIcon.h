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
 * frame extends past the node by the shadow's margin (setShadowMargin(), a whole number of units,
 * set by the owner from the tile size): the icon in the image lands exactly on the node's pixels,
 * so it stays sharp at scale 1 too. The frame is stretched between insets, so it follows the node
 * in the same layout pass (resizing the dock, items growing/shrinking).
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

    /// The shadow's room around the icon, in logical units (see MarginFor()).
    void setShadowMargin(Int32 margin) noexcept;
    Int32 shadowMargin() const noexcept { return m_margin; }

    /// The shadow margin for an icon of @p size units.
    static Int32 MarginFor(SkScalar size) noexcept;

private:
    Int32 m_margin { -1 };
    std::shared_ptr<RImage> m_image;
};

#endif // DOCKICON_H
