#include <Nodes/DockIcon.h>
#include <CZ/AK/Events/AKBakeEvent.h>
#include <CZ/Ream/RImage.h>
#include <CZ/Ream/RPass.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/skia/core/SkCanvas.h>
#include <CZ/skia/core/SkImage.h>
#include <CZ/skia/core/SkSamplingOptions.h>
#include <algorithm>

DockIcon::DockIcon(std::shared_ptr<RImage> image, AKNode *parent) noexcept :
    AKBakeable(parent),
    m_image(image)
{
    SkRegion empty;
    setInputRegion(&empty);
}

void DockIcon::setImage(std::shared_ptr<RImage> image) noexcept
{
    if (m_image == image)
        return;

    m_image = image;
    addDamage(AK_IRECT_INF);
}

void DockIcon::setIconOpacity(SkScalar opacity) noexcept
{
    setOpacity(opacity);
    shadow.setOpacity(opacity);
}

void DockIcon::bakeEvent(const AKBakeEvent &e)
{
    if (e.damage.isEmpty())
        return;

    auto pass { e.surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas *c { pass->getCanvas() };
    c->clear(SK_ColorTRANSPARENT);

    if (!m_image || worldRect().isEmpty())
        return;

    auto image { m_image->skImage() };

    if (!image)
        return;

    // Contained and centered.
    const SkScalar w { SkScalar(worldRect().width()) }, h { SkScalar(worldRect().height()) };
    const SkScalar s { std::min(w / image->width(), h / image->height()) };
    const SkRect dst { SkRect::MakeXYWH((w - image->width() * s) / 2.f, (h - image->height() * s) / 2.f,
                                        image->width() * s, image->height() * s) };

    SkPaint paint;
    paint.setAntiAlias(true);
    c->drawImageRect(image, SkRect::MakeIWH(image->width(), image->height()), dst,
                     SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNearest), &paint, SkCanvas::kFast_SrcRectConstraint);
}
