#ifndef UTILS_H
#define UTILS_H

#include <include/core/SkImage.h>

class QPixmap;
class QString;
class QWidget;

class Utils
{
public:
    static sk_sp<SkImage> skImageFromPixmap(const QPixmap &pixmap) noexcept;
    static QPixmap iconFromName(const QString &name, int size) noexcept;
    static QPixmap iconApplyShadow(const QPixmap &pixmap, int size) noexcept;
    static QPixmap createThumbnail(const QPixmap &pixmap) noexcept;

    static QString desktopFileFromAppId(const QString &appId) noexcept;
    static QString appIdFromDesktopFile(const QString &desktopFilePath) noexcept;

    static void makeWidgetTransparent(QWidget *widget) noexcept;
};

#endif // UTILS_H
