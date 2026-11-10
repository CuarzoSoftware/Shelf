#include <Core/Assets.h>
#include <Core/Log.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/RPass.h>
#include <CZ/Ream/RImage.h>
#include <CZ/skia/core/SkCanvas.h>
#include <CZ/skia/core/SkRRect.h>
#include <CZ/skia/effects/SkImageFilters.h>
#include <Core/Shelf.h>
#include <CZ/Marco/MScreen.h>
#include <cmath>
#include <CZ/Ream/RCore.h>
#include <CZ/Ream/RDevice.h>
#include <CZ/XDG/XDGKit.h>
#include <CZ/XDG/XDGDesktopEntry.h>
#include <drm_fourcc.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

void Assets::init() noexcept
{
    auto ream { RCore::Get() };
    const auto &formats { ream->mainDevice()->textureFormats().formats() };
    auto fmt { formats.find(DRM_FORMAT_ABGR8888) };

    if (fmt != formats.end())
        m_format = *fmt;
    else
        Log(CZError, CZLN, "ABGR8888 isn't a texture format of the main device: icons won't load");
}

std::shared_ptr<RImage> Assets::loadIcon(const std::string &name) noexcept
{
    auto &xdgkit { GetShelf()->xdgkit };

    if (name.empty() || !xdgkit || m_format.format() == 0)
        return {};

    // Absolute paths are allowed in .desktop Icon keys.
    if (name.front() == '/')
        return RImage::LoadFile(name, m_format, { DOCK_ICON_LOAD_SIZE, DOCK_ICON_LOAD_SIZE });

    const auto *icon { xdgkit->iconThemeManager().findIcon(
        name, DOCK_ICON_LOAD_SIZE, 1, XDGIcon::PNG | XDGIcon::SVG, { DOCK_ICON_THEME, "" }) };

    if (!icon)
        return {};

    std::shared_ptr<RImage> image;

    if (icon->extensions() & XDGIcon::SVG)
        image = RImage::LoadFile(icon->getPath(XDGIcon::SVG), m_format, { DOCK_ICON_LOAD_SIZE, DOCK_ICON_LOAD_SIZE });

    if (!image && (icon->extensions() & XDGIcon::PNG))
        image = RImage::LoadFile(icon->getPath(XDGIcon::PNG), m_format, { DOCK_ICON_LOAD_SIZE, DOCK_ICON_LOAD_SIZE });

    return image;
}

std::shared_ptr<RImage> Assets::themedIcon(const std::vector<std::string> &names) noexcept
{
    for (const auto &name : names)
    {
        auto it { m_icons.find(name) };

        if (it == m_icons.end())
            it = m_icons.emplace(name, loadIcon(name)).first;

        if (it->second)
            return it->second;
    }

    return {};
}

std::string Assets::desktopEntryIcon(const std::string &appId) noexcept
{
    auto &xdgkit { GetShelf()->xdgkit };
    auto entry { xdgkit ? xdgkit->desktopEntry(appId) : nullptr };
    return entry ? entry->icon : std::string {};
}

std::shared_ptr<RImage> Assets::appIcon(const std::string &appId) noexcept
{
    if (auto it { m_appIcons.find(appId) }; it != m_appIcons.end())
        return it->second;

    std::vector<std::string> names;

    if (auto icon { desktopEntryIcon(appId) }; !icon.empty())
        names.emplace_back(icon);

    if (!appId.empty())
        names.emplace_back(appId);

    names.emplace_back("application-default-icon");
    names.emplace_back("application-x-executable");

    auto image { themedIcon(names) };

    if (!image)
        Log(CZWarning, CZLN, "No icon found for app id '{}'", appId);

    m_appIcons[appId] = image;
    return image;
}

std::shared_ptr<RImage> Assets::windowIcon() noexcept
{
    return themedIcon({ "preferences-system-windows", "window", "application-x-executable" });
}

std::shared_ptr<RImage> Assets::deskIcon() noexcept
{
    return themedIcon({ "system-file-manager", "org.gnome.Nautilus", "file-manager", "user-home" });
}

float Assets::ShadowMargin() noexcept
{
    // The shadow keeps the proportions it has at the default tile size.
    return DOCK_ICON_SHADOW_RADIUS / SkScalar(DOCK_TILE_SIZE_DEFAULT);
}

std::shared_ptr<RImage> Assets::shadowedIcon(const std::shared_ptr<RImage> &icon) noexcept
{
    if (!icon)
        return {};

    // The largest screen scale (sharp everywhere).
    Int32 scale { 1 };
    for (MScreen *s : GetShelf()->marco->screens())
        scale = std::max(scale, s->props().scale);

    if (auto it { m_shadowed.find(icon.get()) }; it != m_shadowed.end() && !it->second.icon.expired() && it->second.scale == scale)
        return it->second.image;

    std::erase_if(m_shadowed, [](const auto &pair){ return pair.second.icon.expired(); });

    auto source { icon->skImage() };

    if (!source)
        return icon;

    // In logical units of the largest tile size (the surface applies the scale).
    const SkScalar size { SkScalar(DOCK_TILE_SIZE_MAX) };
    const SkScalar margin { std::ceil(size * ShadowMargin()) };
    const SkScalar ratio { size / SkScalar(DOCK_TILE_SIZE_DEFAULT) };
    const SkScalar sigma { DOCK_ICON_SHADOW_RADIUS * ratio / 3.f };
    const SkScalar dy { DOCK_ICON_SHADOW_OFFSET_Y * ratio };

    auto surface { RSurface::Make(SkISize::Make(Int32(size + 2 * margin), Int32(size + 2 * margin)), scale, true) };

    if (!surface)
        return icon;

    {
        auto pass { surface->beginPass(RPassCap_SkCanvas) };
        SkCanvas *c { pass->getCanvas() };
        c->clear(SK_ColorTRANSPARENT);

        const SkScalar s { std::min(size / source->width(), size / source->height()) };
        const SkRect dst { SkRect::MakeXYWH(margin + (size - source->width() * s) / 2.f, margin + (size - source->height() * s) / 2.f,
                                            source->width() * s, source->height() * s) };

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setImageFilter(SkImageFilters::DropShadow(0.f, dy, sigma, sigma, DOCK_ICON_SHADOW_COLOR, nullptr));
        c->drawImageRect(source, SkRect::MakeIWH(source->width(), source->height()), dst,
                         SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear), &paint, SkCanvas::kFast_SrcRectConstraint);
    }

    auto image { surface->image() };
    m_shadowed[icon.get()] = { icon, scale, image };
    return image;
}

std::shared_ptr<RImage> Assets::trashIcon(bool full) noexcept
{
    if (full)
        return themedIcon({ "user-trash-full", "user-trash" });

    return themedIcon({ "user-trash", "user-trash-full" });
}

const Assets::DockFrame &Assets::dockFrame(Int32 scale, CZColorScheme scheme, Int32 height, Int32 radius, const SkIRect &margins) noexcept
{
    scale = std::max(1, scale);
    const Key key { scale, scheme, height, radius, margins.fLeft, margins.fTop, margins.fRight, margins.fBottom };

    if (auto it { m_dockFrames.find(key) }; it != m_dockFrames.end())
        return it->second;

    // Narrowest dock whose middle column is past both rounded corners and their shadow falloff.
    // (Also the dock's minimum width, see DockMetrics::minWidth.)
    const Int32 width { 2 * (radius + DOCK_SHADOW_EXTENT) + 1 };
    const SkISize size { margins.fLeft + width + margins.fRight, margins.fTop + height + margins.fBottom };

    DockFrame &frame { m_dockFrames[key] };
    frame.center = SkIRect::MakeXYWH(margins.fLeft + radius + DOCK_SHADOW_EXTENT, margins.fTop + height / 2, 1, 1);

    auto surface { RSurface::Make(size, scale, true) };

    if (!surface)
    {
        Log(CZError, CZLN, "Failed to create the dock frame surface");
        return frame;
    }

    auto pass { surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas *c { pass->getCanvas() };
    c->clear(SK_ColorTRANSPARENT);

    const SkRect rect { SkRect::MakeXYWH(margins.fLeft, margins.fTop, width, height) };
    const SkRRect rrect { SkRRect::MakeRectXY(rect, radius, radius) };

    SkPaint paint;
    paint.setAntiAlias(true);

    /* Drop shadow (only outside the dock: its interior is cleared below) */
    const SkScalar sigma { DOCK_SHADOW_BLUR * 0.5f };
    paint.setImageFilter(SkImageFilters::DropShadowOnly(0.f, DOCK_SHADOW_OFFSET_Y, sigma, sigma, DOCK_SHADOW_COLOR, nullptr));
    c->drawRRect(rrect, paint);
    paint.setImageFilter(nullptr);

    paint.setBlendMode(SkBlendMode::kClear);
    c->drawRRect(rrect, paint);
    paint.setBlendMode(SkBlendMode::kSrcOver);

    /* Hairline borders: one physical pixel each, the dark one on the outer edge, the light one
     * just inside it. */
    const SkScalar px { 1.f / scale };
    paint.setStroke(true);
    paint.setStrokeWidth(px);

    SkRRect outer { rrect };
    outer.inset(px * 0.5f, px * 0.5f);
    paint.setColor(DOCK_OUTER_BORDER.resolve(scheme));
    c->drawRRect(outer, paint);

    SkRRect inner { rrect };
    inner.inset(px * 1.5f, px * 1.5f);
    paint.setColor(DOCK_INNER_BORDER.resolve(scheme));
    c->drawRRect(inner, paint);

    pass.reset();
    frame.image = surface->image();
    return frame;
}

std::shared_ptr<RImage> Assets::dividerImage() noexcept
{
    if (m_dividerImage)
        return m_dividerImage;

    auto surface { RSurface::Make(SkISize(1, 3), 1, true) };

    if (!surface)
    {
        Log(CZError, CZLN, "Failed to create the divider surface");
        return {};
    }

    auto pass { surface->beginPass(RPassCap_SkCanvas) };
    pass->getCanvas()->clear(SK_ColorWHITE);
    pass.reset();
    m_dividerImage = surface->image();
    return m_dividerImage;
}
