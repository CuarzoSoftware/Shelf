#ifndef THEME_H
#define THEME_H

#include <Marco/MTheme.h>

using namespace AK;

class Theme : public MTheme
{
public:
    using MTheme::MTheme;

    // COLORS
    static constexpr SkColor DOCK_BORDER_COLOR { SkColorSetARGB(70, 0, 0, 0) };
    static constexpr SkColor DOCK_SHADOW_COLOR { SkColorSetARGB(60, 0, 0, 0) };

    // ANIMATIONS
    static constexpr UInt32 dockSlideUpMs { 300 };
    static constexpr UInt32 dockSlideDownMs { 300 };

    // REFERENCE
    static constexpr inline SkScalar MAX_DOCK_SHADOW_RADIUS { 46.f };
    static constexpr inline SkScalar MAX_DOCK_BORDER_RADIUS { 64.f };
    static constexpr inline SkScalar MAX_DOCK_HEIGHT { 146.f };
    static constexpr inline SkScalar MIN_DOCK_HEIGHT { 24.f };
    static constexpr inline SkScalar MAX_DOT_SIZE { 4.f };
    static constexpr inline SkScalar MIN_DOT_SIZE { 3.f };
    static constexpr inline SkScalar BORDER_SIZE { 1.f };
    static constexpr inline SkScalar DOCK_MARGIN_BOTTOM { 5.f };

    // RELATIVE TO HEIGHT
    static constexpr inline SkScalar dockShadowRadius(SkScalar dockHeight) { return (dockHeight * MAX_DOCK_SHADOW_RADIUS) / MAX_DOCK_HEIGHT; }
    static constexpr inline SkScalar dockBorderRadius(SkScalar dockHeight) { return (dockHeight * MAX_DOCK_BORDER_RADIUS) / MAX_DOCK_HEIGHT; }
    static constexpr inline SkScalar dockTextureHeight(SkScalar dockHeight) { return dockHeight + SkScalarCeilToScalar(dockShadowRadius(dockHeight)) + DOCK_MARGIN_BOTTOM; }

    // DOCK 3 PATCH
    static sk_sp<SkImage> dockTexture(SkScalar height, Int32 scale, SkRect &side, SkRect &center) noexcept;
};

#endif // THEME_H
