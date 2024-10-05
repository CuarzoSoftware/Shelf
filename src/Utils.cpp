#include <include/core/SkPixmap.h>
#include <Utils.h>
#include <QPixmap>

sk_sp<SkImage> Utils::skImageFromPixmap(const QPixmap &pixmap) noexcept
{
    QImage img { pixmap.toImage() };
    SkImageInfo imageInfo = SkImageInfo::Make(img.width(), img.height(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
    return SkImage::MakeRasterCopy(SkPixmap(imageInfo, img.bits(), img.bytesPerLine()));
}
