#ifndef TYPES_H
#define TYPES_H

#include <CZ/Core/Cuarzo.h>
#include <CZ/Core/CZAdaptive.h>
#include <CZ/skia/core/SkColor.h>
#include <CZ/skia/core/SkScalar.h>
#include <cmath>

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
constexpr Int32 DOCK_SCREEN_SIDE_MARGIN { 8 };   // min space left on each side when the dock fills the screen width
constexpr Int32 DOCK_TOOLTIP_SCREEN_MARGIN { 12 }; // min space between tooltips/menus and the screen's left and right edges

// --- Shadow ----------------------------------------------------------------------------------

constexpr Int32   DOCK_SHADOW_BLUR      { 20 };  // CSS-like blur radius (sigma = blur / 2)
constexpr Int32   DOCK_SHADOW_OFFSET_Y  { 4 };
constexpr SkColor DOCK_SHADOW_COLOR     { 0x40000000 };

// Distance the blurred shadow extends past the dock rect (~3 sigma).
constexpr Int32   DOCK_SHADOW_EXTENT    { (DOCK_SHADOW_BLUR * 3) / 2 };

/**
 * @brief Dock geometry for a tile (icon) size. Everything scales with it, keeping the proportions
 *        of the macOS Dock at its default tile size (48).
 *
 * The tile size can be fractional: a dock that doesn't fit the screen shrinks continuously while
 * its items grow in or shrink out (see Dock::fitToScreen()). Round where whole units are needed.
 */
struct DockMetrics
{
    SkScalar tile;
    SkScalar paddingTop;    // above the icons
    SkScalar paddingBottom; // below the icons (holds the running indicators)
    SkScalar paddingSide;   // at both ends
    SkScalar spacing;       // between icons
    SkScalar height;
    SkScalar radius;
    SkScalar dividerSlot;   // width of the divider's (draggable) slot
    SkScalar dividerHeight; // length of the divider line
    SkScalar minWidth;      // see Make()

    /// @p tile clamped to DOCK_TILE_SIZE_MIN..MAX.
    static constexpr Int32 ClampTile(Int32 tile) noexcept
    {
        return tile < DOCK_TILE_SIZE_MIN ? DOCK_TILE_SIZE_MIN : (tile > DOCK_TILE_SIZE_MAX ? DOCK_TILE_SIZE_MAX : tile);
    }

    static DockMetrics Make(SkScalar tile) noexcept
    {
        tile = tile < DOCK_TILE_SIZE_MIN ? SkScalar(DOCK_TILE_SIZE_MIN) : (tile > DOCK_TILE_SIZE_MAX ? SkScalar(DOCK_TILE_SIZE_MAX) : tile);
        const auto scaled { [tile](SkScalar atDefault) { return atDefault * tile / SkScalar(DOCK_TILE_SIZE_DEFAULT); } };

        DockMetrics m {};
        m.tile          = tile;
        m.paddingTop    = scaled(5);
        m.paddingBottom = scaled(8);
        m.paddingSide   = scaled(6);
        m.spacing       = scaled(6);
        m.height        = m.paddingTop + tile + m.paddingBottom;
        m.radius        = scaled(18);
        m.dividerSlot   = scaled(12);
        m.dividerHeight = scaled(36);
        // Narrowest dock whose 9-patch frame doesn't overlap: both rounded corners and their
        // shadow falloff, plus the stretched middle column.
        m.minWidth      = 2 * (std::round(m.radius) + DOCK_SHADOW_EXTENT) + 1;
        return m;
    }
};

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

// --- Hover labels --------------------------------------------------------------------------

// Grace period after leaving an item before its label hides, so moving across the gap between two
// items switches labels instead of hiding and showing them.
constexpr UInt32 DOCK_LABEL_HIDE_DELAY_MS { 120 };
constexpr SkScalar DOCK_LABEL_FONT_SIZE { 14.f };

// --- Minimize ---------------------------------------------------------------------------------

// Minimized window items grow in (and shrink out when restored) while the compositor animates the
// window into (out of) them. Matches cz-crystals' minimize animation.
constexpr UInt32 DOCK_MINIMIZE_MS       { 420 };
constexpr UInt32 DOCK_PIN_GAP_MS { 220 }; // Opening/closing the slot for a dropped .desktop file

// --- Reordering (drag & drop) -------------------------------------------------------------------

// Pointer travel (units) with the button held on an app before it starts being dragged.
constexpr SkScalar DOCK_DRAG_THRESHOLD  { 4.f };
// Icons sliding aside to make room for the dragged one.
constexpr UInt32   DOCK_REORDER_MS      { 220 };
// Private MIME type of dragged dock apps (the data is the app id).
constexpr UInt32 DOCK_DROP_FLY_MS { 320 };     // An app dropped away from the dock flying back to its slot
constexpr UInt32 DOCK_DROP_FLY_HOLD_MS { 80 }; // Crystals holds the drag icon this long after a dock drop (its Seat/DND.cpp)
#define DOCK_APP_MIME_TYPE "application/x-cz-shelf-app"

// --- Launch bounce ---------------------------------------------------------------------------

// One bounce of the icon of an app being launched (up and back down).
constexpr UInt32   DOCK_BOUNCE_MS         { 700 };
// Peak height, relative to the tile size.
constexpr SkScalar DOCK_BOUNCE_HEIGHT     { 0.6f };
// Stop bouncing if the app hasn't shown a window by then.
constexpr UInt32   DOCK_BOUNCE_TIMEOUT_MS { 10000 };

// --- Icons and indicators --------------------------------------------------------------------

#define DOCK_ICON_THEME "WhiteSur-light"

// Icons are loaded once at this size (px) so they stay sharp up to DOCK_TILE_SIZE_MAX at 2x.
constexpr Int32 DOCK_ICON_LOAD_SIZE     { 256 };
constexpr SkScalar DOCK_ITEM_DARKEN     { 0.9f };  // Color factor of a pressed item / drop target
constexpr SkColor  DOCK_ICON_SHADOW_COLOR    { 0x38000000 }; // Faint shadow following each icon's shape
constexpr SkScalar DOCK_ICON_SHADOW_RADIUS   { 6.f };
constexpr Int32    DOCK_ICON_SHADOW_OFFSET_Y { 1 };

// Running indicator under app icons.
inline const CZ::CZAdaptiveColor DOCK_INDICATOR_COLOR { 0xB3000000, 0xE6FFFFFF };

// --- Forward declarations --------------------------------------------------------------------

class Shelf;
class Dock;
class DockBackground;
class DockFrame;
class DockDivider;
class DockItem;
class LaunchBounce;

Shelf *GetShelf() noexcept;

#endif // TYPES_H
