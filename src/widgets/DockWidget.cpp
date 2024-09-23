#include <widgets/DockWidget.h>
#include <widgets/Dock.h>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <controller/Shelf.h>

DockWidget::DockWidget(Dock &dock) noexcept : QFrame(&dock),
    dock(dock)
{
    shadowFx->setBlurRadius(64);
    shadowFx->setOffset(0, 8);
    shadowFx->setColor(QColor(0, 0, 0, 64));
    setGraphicsEffect(shadowFx);

    QStringList icons {
        "file-manager",
        "firefox",
        "spotify",
        "code",
        "codeblocks",
        "discord",
        "foot",
        "qtcreator",
        "gimp",
        "inkscape",
        "com.valvesoftware.Steam"
    };

    for (auto &ic : icons)
    {
        QIcon::setThemeSearchPaths({"/usr/share/icons", "/usr/local/share/icons", Shelf::get().homeDir.filePath(".local/share/icons")});
        QIcon::setThemeName("WhiteSur");
        QIcon::setFallbackThemeName("Paper");
        QIcon icon = QIcon::fromTheme(ic);

        if (!icon.isNull())
        {
            QLabel *label = new QLabel(this);
            QGraphicsDropShadowEffect *fx { new QGraphicsDropShadowEffect(label)};
            fx->setBlurRadius(4);
            fx->setOffset(0, 1);
            fx->setColor(QColor(0, 0, 0, 64));
            label->setGraphicsEffect(fx);
            label->setScaledContents(true);
            label->setPixmap(icon.pixmap(512, 512));
            label->setFixedSize(36, 36);
            layout->addWidget(label);
        }
    }
}
