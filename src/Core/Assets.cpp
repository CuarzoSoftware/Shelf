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
#include <CZ/AssetBroker/ABCore.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

void Assets::init() noexcept
{
    // Uses the cz-asset-broker service if it's running (icons shared with other apps as DMA-bufs,
    // looked up and rasterized once), otherwise loads them here
    m_ab = ABCore::GetOrMake();

    if (!m_ab)
    {
        Log(CZError, CZLN, "Failed to create the ABCore: icons won't load");
        return;
    }

    // Local lookups reuse Shelf's XDGKit (also used for .desktop entries)
    m_ab->setXDGKit(GetShelf()->xdgkit);
    Log(CZInfo, CZLN, "Icons from {}", m_ab->remote() ? "the asset broker service" : "this process");
}

ABIconKey Assets::iconKey(std::vector<std::string> names, const std::string &appId) noexcept
{
    ABIconKey key;
    key.names = std::move(names);
    key.appId = appId;
    key.size = DOCK_ICON_LOAD_SIZE;
    key.themes = { DOCK_ICON_THEME, "" };
    return key;
}

std::shared_ptr<RImage> Assets::load(const ABIconKey &key) noexcept
{
    if (!m_ab)
        return {};

    auto image { m_ab->icon(key, { DOCK_ICON_LOAD_SIZE, DOCK_ICON_LOAD_SIZE }) };

    if (image)
    {
        std::erase_if(m_sources, [](const auto &pair){ return pair.second.image.expired(); });
        m_sources[image.get()] = { image, key };
    }

    return image;
}

std::shared_ptr<RImage> Assets::sized(const std::shared_ptr<RImage> &icon, Int32 pixels, Int32 scale) noexcept
{
    CZ_UNUSED(scale) // pixels are already physical: the same artwork for every scale

    if (!icon || !m_ab)
        return icon;

    auto src { m_sources.find(icon.get()) };

    if (src == m_sources.end() || src->second.image.expired())
        return icon;

    // Same lookup (so an SVG is rendered again from the same file: other sizes of a theme may be
    // different artwork, e.g. padded for panels), fitted in the new size. Shared and cached by the
    // ABCore (and the service).
    auto image { m_ab->icon(src->second.key, { std::max(1, pixels), std::max(1, pixels) }) };
    return image ? image : icon;
}

AKImageFrame::ImageProvider Assets::provider(std::shared_ptr<RImage> icon) noexcept
{
    if (!icon)
        return nullptr;

    return [this, icon](Int32 pixels, Int32 scale){ return sized(icon, pixels, scale); };
}

std::shared_ptr<RImage> Assets::themedIcon(const std::vector<std::string> &names) noexcept
{
    std::string cacheKey;

    for (const auto &name : names)
        cacheKey.append(name).push_back('\n');

    if (auto it { m_icons.find(cacheKey) }; it != m_icons.end())
        return it->second;

    // One lookup with fallbacks (cached, including misses)
    auto image { load(iconKey(names)) };
    m_icons[cacheKey] = image;
    return image;
}

std::shared_ptr<RImage> Assets::appIcon(const std::string &appId) noexcept
{
    if (auto it { m_appIcons.find(appId) }; it != m_appIcons.end())
        return it->second;

    // The Icon of the app's .desktop entry (resolved by the service in remote mode), else the
    // app id itself as an icon name, else the generic app icon
    std::vector<std::string> names;

    if (!appId.empty())
        names.emplace_back(appId);

    names.emplace_back("application-default-icon");
    names.emplace_back("application-x-executable");

    auto image { load(iconKey(std::move(names), appId)) };

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

std::shared_ptr<RImage> Assets::shadowedIcon(const std::shared_ptr<RImage> &icon, Int32 pixels, Int32 iconPixels, Int32 scale) noexcept
{
    if (!icon)
        return {};

    pixels = std::max(pixels, 8);
    iconPixels = std::clamp(iconPixels, 1, pixels);

    if (auto it { m_shadowed.find({ icon.get(), pixels, iconPixels, scale }) }; it != m_shadowed.end() && !it->second.icon.expired())
        return it->second.image;

    // Themed icons: drawn with their shadow by the AssetBroker (shared with other apps if the
    // service is running), from their file at the exact size
    if (m_ab)
        if (auto src { m_sources.find(icon.get()) }; src != m_sources.end() && !src->second.image.expired())
        {
            const ABShadow shadow { (pixels - iconPixels) / 2, ShadowMargin(),
                0.f, DOCK_ICON_SHADOW_OFFSET_Y / SkScalar(DOCK_TILE_SIZE_DEFAULT), DOCK_ICON_SHADOW_COLOR };

            if (auto image { m_ab->icon(src->second.key, { pixels, pixels }, nullptr, &shadow) })
                return image;
        }

    std::erase_if(m_shadowed, [](const auto &pair){ return pair.second.icon.expired(); });

    // Other images: drawn here. The image covers the icon plus the shadow margin on each side, in pixels (scale 1): the icon
    // is drawn at its exact size (never resampled), at a whole pixel offset (a half pixel one would
    // blur it, noticeable at scale 1).
    const Int32 size { iconPixels };
    const SkScalar margin { SkScalar((pixels - size) / 2) };
    const SkScalar ratio { SkScalar(size) / SkScalar(DOCK_TILE_SIZE_DEFAULT) };
    const SkScalar sigma { DOCK_ICON_SHADOW_RADIUS * ratio / 3.f };
    const SkScalar dy { DOCK_ICON_SHADOW_OFFSET_Y * ratio };

    auto source { sized(icon, size, scale)->skImage() };
    auto surface { source ? RSurface::Make(SkISize::Make(pixels, pixels), 1, true) : nullptr };

    if (!surface)
        return icon;

    {
        auto pass { surface->beginPass(RPassCap_SkCanvas) };
        SkCanvas *c { pass->getCanvas() };
        c->clear(SK_ColorTRANSPARENT);

        const SkScalar s { std::min(SkScalar(size) / source->width(), SkScalar(size) / source->height()) };
        const SkRect dst { SkRect::MakeXYWH(margin + (size - source->width() * s) / 2.f, margin + (size - source->height() * s) / 2.f,
                                            source->width() * s, source->height() * s) };

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setImageFilter(SkImageFilters::DropShadow(0.f, dy, sigma, sigma, DOCK_ICON_SHADOW_COLOR, nullptr));
        c->drawImageRect(source, SkRect::MakeIWH(source->width(), source->height()), dst,
                         SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear), &paint, SkCanvas::kFast_SrcRectConstraint);
    }

    auto image { surface->image() };
    m_shadowed[{ icon.get(), pixels, iconPixels, scale }] = { icon, image };
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
