#ifndef UTILS_H
#define UTILS_H

#include <include/core/SkImage.h>

class QPixmap;

class Utils
{
public:
    static sk_sp<SkImage> skImageFromPixmap(const QPixmap &pixmap) noexcept;
};

#endif // UTILS_H
