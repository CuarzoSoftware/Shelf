#ifndef TYPES_H
#define TYPES_H

#include <CZ/Core/Cuarzo.h>
#include <CZ/Core/CZAdaptive.h>
#include <CZ/skia/core/SkColor.h>

using namespace CZ;

/*
 * Dock metrics, colors and timings, modeled after the macOS Monterey Dock.
 *
 * Apple publishes no specification for the Dock, so the visual values are measured/estimated from
 * recreations (Plank "Big Sur" theme, macos-web) and screenshots. The behaviour values come from
 * the documented `defaults` keys (tilesize 48, ~0.5 s show/hide animation, 0.2 s autohide delay).
 * All sizes are in logical units (points).
 */

// --- Geometry --------------------------------------------------------------------------------

constexpr Int32 DOCK_TILE_SIZE_DEFAULT  { 48 };  // `tilesize` default
constexpr Int32 DOCK_TILE_SIZE_MIN      { 16 };  // Same range as the macOS Dock size slider
constexpr Int32 DOCK_TILE_SIZE_MAX      { 128 };
constexpr Int32 DOCK_SCREEN_GAP         { 4 };   // floating gap between the dock and the screen edge

/**
 * @brief Dock geometry for a tile (icon) size. Everything scales with it, keeping the proportions
 *        of the macOS Dock at its default tile size (48).
 */
struct DockMetrics
{
    Int32 tile;
    Int32 paddingTop;       // above the icons
    Int32 paddingBottom;    // below the icons (holds the running indicators)
    Int32 paddingSide;      // at both ends
    Int32 spacing;          // between icons
    Int32 height;
    Int32 radius;
    Int32 dividerSlot;      // width of the divider's (draggable) slot
    Int32 dividerHeight;    // length of the divider line

    static constexpr DockMetrics Make(Int32 tile) noexcept
    {
        tile = tile < DOCK_TILE_SIZE_MIN ? DOCK_TILE_SIZE_MIN : (tile > DOCK_TILE_SIZE_MAX ? DOCK_TILE_SIZE_MAX : tile);
        const auto scaled { [tile](Int32 atDefault) { return (atDefault * tile + DOCK_TILE_SIZE_DEFAULT / 2) / DOCK_TILE_SIZE_DEFAULT; } };

        DockMetrics m {};
        m.tile          = tile;
        m.paddingTop    = scaled(7);
        m.paddingBottom = scaled(6);
        m.paddingSide   = scaled(6);
        m.spacing       = scaled(6);
        m.height        = m.paddingTop + tile + m.paddingBottom;
        m.radius        = scaled(18);
        m.dividerSlot   = scaled(12);
        m.dividerHeight = scaled(36);
        return m;
    }
};

// --- Shadow ----------------------------------------------------------------------------------

constexpr Int32   DOCK_SHADOW_BLUR      { 20 };  // CSS-like blur radius (sigma = blur / 2)
constexpr Int32   DOCK_SHADOW_OFFSET_Y  { 4 };
constexpr SkColor DOCK_SHADOW_COLOR     { 0x40000000 };

// Distance the blurred shadow extends past the dock rect (~3 sigma).
constexpr Int32   DOCK_SHADOW_EXTENT    { (DOCK_SHADOW_BLUR * 3) / 2 };

// --- Fill and hairline borders (light, dark) -------------------------------------------------

// Tint drawn over the background blur (vibrancy enabled). None: the compositor's blur material
// already carries the light/dark tint.
inline const CZ::CZAdaptiveColor DOCK_TINT_COLOR      { 0x00000000, 0x00000000 };
// Opaque-ish fill when the compositor doesn't support background blur.
inline const CZ::CZAdaptiveColor DOCK_FALLBACK_COLOR  { 0xF2ECECEC, 0xF2282828 };
// Divider between the pinned apps and the trash / minimized windows.
inline const CZ::CZAdaptiveColor DOCK_DIVIDER_COLOR   { 0x38000000, 0x4DFFFFFF };
// One physical pixel each: a dark outer line and a light inner line.
inline const CZ::CZAdaptiveColor DOCK_OUTER_BORDER    { 0x26000000, 0x99000000 };
inline const CZ::CZAdaptiveColor DOCK_INNER_BORDER    { 0x66FFFFFF, 0x33FFFFFF };

// --- Auto-hide -------------------------------------------------------------------------------

// Units of the dock left on screen while hidden, so it can still get pointer enter events.
constexpr Int32  DOCK_HIDDEN_VISIBLE    { 2 };
// Units of the surface kept below the screen edge while shown, so the pointer reaching the bottom
// edge (under the floating dock) doesn't count as leaving it.
constexpr Int32  DOCK_SHOWN_UNDERSCAN   { 10 };
// `autohide-delay` default: the pointer must rest this long on the hidden dock before it shows.
constexpr UInt32 DOCK_SHOW_DELAY_MS     { 200 };
// Show/hide slide duration.
constexpr UInt32 DOCK_SLIDE_MS          { 250 };
// Ease-in-out timing curve (CAMediaTimingFunction easeInEaseOut).
constexpr Float64 DOCK_EASE[4]          { 0.42, 0.0, 0.58, 1.0 };

// --- Icons and indicators --------------------------------------------------------------------

#define DOCK_ICON_THEME "WhiteSur-light"

// Icons are loaded once at this size (px) so they stay sharp up to DOCK_TILE_SIZE_MAX at 2x.
constexpr Int32 DOCK_ICON_LOAD_SIZE     { 256 };

// Running indicator under app icons.
inline const CZ::CZAdaptiveColor DOCK_INDICATOR_COLOR { 0xB3000000, 0xE6FFFFFF };

// --- Forward declarations --------------------------------------------------------------------

class Shelf;
class Dock;
class DockBackground;
class DockFrame;
class DockDivider;
class DockItem;

Shelf *GetShelf() noexcept;

#endif // TYPES_H
