#include <include/core/SkPixmap.h>
#include <core/Shelf.h>
#include <core/Utils.h>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

sk_sp<SkImage> Utils::skImageFromPixmap(const QPixmap &pixmap) noexcept
{
    const QImage img { pixmap.toImage() };
    SkImageInfo imageInfo = SkImageInfo::Make(img.width(), img.height(), kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    return SkImage::MakeRasterCopy(SkPixmap(imageInfo, img.bits(), img.bytesPerLine()));
}

QPixmap Utils::iconFromName(const QString &name, int size) noexcept
{
    QIcon icon = QIcon::fromTheme(name);

    if (icon.isNull())
        return QPixmap();

    return icon.pixmap(size, size);
}

QPixmap Utils::iconApplyShadow(const QPixmap &pixmap, int size) noexcept
{
    if (pixmap.isNull())
        return pixmap;

    static QWidget widget{};
    widget.setFixedSize(size, size);
    makeWidgetTransparent(&widget);

    static QLabel label {&widget};
    label.setScaledContents(true);
    label.setFixedSize(size - 4, size - 4);
    label.move(2, 2);
    label.setPixmap(pixmap);

    static QGraphicsDropShadowEffect shadowFx {&label};
    shadowFx.setBlurRadius(8);
    shadowFx.setColor(QColor(0, 0, 0, 72));
    shadowFx.setOffset(0, 1);
    label.setGraphicsEffect(&shadowFx);

    return widget.grab();
}

QPixmap Utils::createThumbnail(const QPixmap &pixmap) noexcept
{
    QPixmap shadow { iconApplyShadow(pixmap, 64) };
    QPixmap pix(128, 96);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setBrush(QColor(240, 240, 240, 255));
    QPen pen;
    pen.setColor(QColor(0, 0, 0, 64));
    pen.setWidthF(0.5);
    p.setPen(pen);
    p.setRenderHint(QPainter::RenderHint::Antialiasing);
    p.drawRoundedRect(1, 1, pix.width() - 2, pix.height() - 2, 8, 8);
    p.drawPixmap(16, 48, 32, 32, shadow);
    p.end();
    return pix;
}

QString Utils::desktopFileFromAppId(const QString &appId) noexcept
{
    // TODO handle / cases

    QString filePath;
    const QString fileName { appId.trimmed() + ".desktop" };
    for (const QDir &appDir : Shelf::get().appsDirs)
    {
        filePath = appDir.filePath(fileName);

        if (QFile::exists(filePath))
            return filePath;
    }

    return QString();
}

QString Utils::appIdFromDesktopFile(const QString &desktopFilePath) noexcept
{
    QString res { desktopFilePath.trimmed() };
    QString pre { "/applications/" };
    const QString suf { ".desktop" };

    int64_t indP { res.lastIndexOf(pre) };

    if (indP < 0)
    {
        pre = "/";
        indP = res.lastIndexOf(pre);
    }

    if (indP >= 0)
        res = res.sliced(indP + pre.length());

    const int64_t indS { res.lastIndexOf(suf) };

    if (indS >= 0)
        res.chop(res.length() - indS);

    return res.replace("/", "-");
}

void Utils::makeWidgetTransparent(QWidget *widget) noexcept
{
    widget->setPalette(Qt::transparent);
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_TranslucentBackground);
}
