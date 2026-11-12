#ifndef ASSETS_H
#define ASSETS_H

#include <Core/Types.h>
#include <CZ/Core/CZColorScheme.h>
#include <CZ/Ream/DRM/RDRMFormat.h>
#include <CZ/skia/core/SkRect.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <CZ/AK/Nodes/AKImageFrame.h>

namespace CZ { class RImage; }

/**
 * @brief Generated (and cached) images and icons shared by every dock.
 */
class Assets
{
public:
    /// 9-patch image of the dock frame: drop shadow plus the outer/inner hairline borders.
    struct DockFrame
    {
        std::shared_ptr<RImage> image;
        SkIRect center; ///< Stretchable 9-patch center (logical units).
    };

    /// Call once Marco (Ream) and XDGKit are ready.
    void init() noexcept;

    /// White 1x3 image for the divider's vertical 3-patch (tinted through the node color).
    std::shared_ptr<RImage> dividerImage() noexcept;

    /**
     * @brief The dock frame for a dock of @p height surrounded by @p margins (the shadow area).
     *
     * The image covers the whole decorations area (dock rect + margins) at its real height and the
     * minimum width; the 9-patch only stretches a 1-unit column in the middle of the straight edges.
     * The dock interior is transparent: the fill is drawn underneath by the vibrancy view.
     */
    const DockFrame &dockFrame(Int32 scale, CZColorScheme scheme, Int32 height, Int32 radius, const SkIRect &margins) noexcept;

    /**
     * @brief The icon of the app with @p appId.
     *
     * Uses the Icon of the app's .desktop entry, else the app id itself as an icon name, else the
     * generic app icon. Never nullptr unless no generic icon could be loaded either.
     */
    std::shared_ptr<RImage> appIcon(const std::string &appId) noexcept;

    /// Generic window icon (minimized windows).
    std::shared_ptr<RImage> windowIcon() noexcept;

    /**
     * @brief @p icon with its drop shadow, pre-rendered @p pixels wide/tall (cached per icon and size).
     *
     * The icon (loaded again at its exact size, see sized()) fills the middle, with a
     * ShadowMargin() border (fraction of the icon size) on every side for the shadow.
     */
    std::shared_ptr<RImage> shadowedIcon(const std::shared_ptr<RImage> &icon, Int32 pixels, Int32 scale) noexcept;

    /**
     * @brief @p icon (loaded by this class) again, at @p pixels for a screen with scale @p scale.
     *
     * SVGs are rendered again at that size. Raster themed icons are looked up again for that size
     * and scale (scaled, antialiased, only if the theme has no such size). Other files
     * are loaded again at that size. Cached. Returns @p icon itself if it's not known.
     */
    std::shared_ptr<RImage> sized(const std::shared_ptr<RImage> &icon, Int32 pixels, Int32 scale) noexcept;

    /// For AKImageFrame::setImageProvider(): @p icon at the size it's drawn at (nullptr for nullptr).
    AKImageFrame::ImageProvider provider(std::shared_ptr<RImage> icon) noexcept;

    /// The shadow border of shadowedIcon(), as a fraction of the icon size.
    static float ShadowMargin() noexcept;

    /// The Desk tile's icon (the file manager's, like Finder's).
    std::shared_ptr<RImage> deskIcon() noexcept;

    /// Trash icon (with papers when @p full).
    std::shared_ptr<RImage> trashIcon(bool full = false) noexcept;

private:
    /// First themed icon found among @p names (cached, including misses).
    std::shared_ptr<RImage> themedIcon(const std::vector<std::string> &names) noexcept;
    std::shared_ptr<RImage> loadIcon(const std::string &name) noexcept;
    std::string desktopEntryIcon(const std::string &appId) noexcept;

    using Key = std::tuple<Int32, CZColorScheme, Int32, Int32, Int32, Int32, Int32, Int32>;
    std::map<Key, DockFrame> m_dockFrames;
    std::shared_ptr<RImage> m_dividerImage;

    RDRMFormat m_format { 0, {} };
    std::unordered_map<std::string, std::shared_ptr<RImage>> m_icons;    // by icon name
    std::unordered_map<std::string, std::shared_ptr<RImage>> m_appIcons; // by app id

    std::shared_ptr<RImage> loadFile(const std::filesystem::path &path, const std::string &iconName = {}) noexcept;

    // Where each loaded icon comes from (to load it at other sizes).
    struct Source
    {
        std::weak_ptr<RImage> image;
        std::string iconName;       ///< Themed icon (looked up again per size and scale)
        std::filesystem::path path; ///< Otherwise, the file
    };
    std::unordered_map<const RImage*, Source> m_sources;
    std::map<std::tuple<std::string, Int32, Int32>, std::shared_ptr<RImage>> m_sized; // By source, size and scale

    struct Shadowed
    {
        std::weak_ptr<RImage> icon;
        std::shared_ptr<RImage> image;
    };

    std::map<std::tuple<const RImage*, Int32, Int32>, Shadowed> m_shadowed; // By icon, size and scale
};

#endif // ASSETS_H
