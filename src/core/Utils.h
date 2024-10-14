#ifndef UTILS_H
#define UTILS_H

#include <include/core/SkImage.h>
#include <core/Constants.h>

class QPixmap;
class QString;
class QWidget;

struct MultiSizeImage
{
    sk_sp<SkImage> hig;
    sk_sp<SkImage> med;
    sk_sp<SkImage> low;

    sk_sp<SkImage> pick(float currentItemSize) noexcept
    {
        if (currentItemSize / ITEM_MAX_SIZE > ICON_SCALE_HIG - (ICON_SCALE_HIG - ICON_SCALE_MED) * 0.9f)
            return hig;
        else if (currentItemSize / ITEM_MAX_SIZE > ICON_SCALE_MED - (ICON_SCALE_MED - ICON_SCALE_LOW) * 0.75f)
            return med;

        return low;
    }
};

class Utils
{
public:
    static struct MultiSizeImage multiSizeImageFromPixmap(const QPixmap &pixmap) noexcept;
    static sk_sp<SkImage> skImageFromPixmap(const QPixmap &pixmap) noexcept;
    static QPixmap iconFromName(const QString &name, int size) noexcept;
    static QPixmap createThumbnail(const QPixmap &pixmap) noexcept;

    static QString desktopFileFromAppId(const QString &appId) noexcept;
    static QString appIdFromDesktopFile(const QString &desktopFilePath) noexcept;

    static void makeWidgetTransparent(QWidget *widget) noexcept;
};

#endif // UTILS_H
