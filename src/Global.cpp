#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>

#include <KDesktopFile>

#include <Global.h>

void Global::loadConfig()
{
    auto homeLocations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

    if (homeLocations.empty())
    {
        qFatal() << "Could not get HOME dir.";
        exit(1);
    }

    homeDir = QDir(homeLocations.front());

    QString val;
    QFile file;
    file.setFileName(homeDir.filePath(".config/Cuarzo/Shelf/config.json"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();

    QJsonDocument conf = QJsonDocument::fromJson(val.toUtf8());
    QJsonArray items = conf["items"].toArray();


    for (const auto &item : items)
    {
        KDesktopFile df("Hola");//item.toString());

        qInfo() << df.fileName();
    }
}
