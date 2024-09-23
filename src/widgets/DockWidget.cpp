#include <widgets/DockWidget.h>
#include <widgets/Dock.h>
#include <QIcon>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

DockWidget::DockWidget(Dock &dock) noexcept : QFrame(&dock),
    dock(dock)
{
    setStyleSheet(".QFrame{background-color: rgba(250,250,250,0.9); border-radius: 18px;}");
    shadowFx->setBlurRadius(64);
    shadowFx->setOffset(0, 0);
    setGraphicsEffect(shadowFx);

    QStringList icons {
        "firefox",
        "spotify",
        "code",
        "foot",
        "nautilus",
        "files",
        "qtcreator",
        "gimp",
        "nemo"
    };

    for (auto &ic : icons)
    {
        QIcon::setThemeSearchPaths({"/usr/share/icons", "/usr/local/share/icons"});
        QIcon::setThemeName("Cosmic");
        QIcon icon = QIcon::fromTheme(ic);

        if (!icon.isNull())
        {
            QLabel *label = new QLabel(this);
            QGraphicsDropShadowEffect *fx { new QGraphicsDropShadowEffect(label)};
            fx->setBlurRadius(16);
            fx->setOffset(0, 2);
            label->setGraphicsEffect(fx);
            label->setScaledContents(true);
            label->setPixmap(icon.pixmap(128, 128));
            label->setFixedSize(36, 36);
            layout->addWidget(label);
        }
    }
}
