#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/effects/SkBlurMaskFilter.h>
#include <include/core/SkCanvas.h>
#include <AK/AKSurface.h>
#include <Theme.h>

sk_sp<SkImage> Theme::dockTexture(SkScalar height, Int32 scale, SkRect &side, SkRect &center) noexcept
{
    const SkScalar shadowRadius { dockShadowRadius(height) };
    const SkScalar borderRadius { dockBorderRadius(height) };
    const SkScalar textureHeight { dockTextureHeight(height) };

    side.fLeft = 0;
    side.fTop = 0;
    side.fBottom = textureHeight;
    side.fRight = shadowRadius + borderRadius;

    center.fTop = 0;
    center.fBottom = textureHeight;
    center.fLeft = side.fRight;
    center.fRight = center.fLeft + 8;

    const SkISize textureSize (center.fRight, side.fBottom);
    auto surface { AKSurface::Make(textureSize, scale) };
    surface->surface()->recordingContext()->asDirectContext()->resetContext();

    SkCanvas *c { surface->surface()->getCanvas() };
    c->scale(scale, scale);
    c->clear(SK_ColorTRANSPARENT);

    const SkRect dockContentRect { SkRect::MakeXYWH(shadowRadius, shadowRadius, borderRadius * 3.f, height) };
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);
    paint.setColor(DOCK_SHADOW_COLOR);
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, shadowRadius/3.f));
    c->drawRoundRect(dockContentRect, borderRadius, borderRadius, paint);

    paint.setMaskFilter(nullptr);
    paint.setColor(SK_ColorWHITE);
    c->drawRoundRect(dockContentRect, borderRadius, borderRadius, paint);

    paint.setColor(DOCK_BORDER_COLOR);
    paint.setStroke(true);
    c->drawRoundRect(dockContentRect, borderRadius, borderRadius, paint);

    surface->surface()->recordingContext()->asDirectContext()->flush();

    return sk_sp<SkImage>(surface->releaseImage());
}
