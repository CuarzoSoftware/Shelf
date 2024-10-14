#ifndef TRASHCONTROLLER_H
#define TRASHCONTROLLER_H

#include <controllers/ItemController.h>
#include <QFileSystemWatcher>

class TrashController final : public ItemController, public QObject
{
public:
    TrashController() noexcept;
    void drawItem(SkCanvas *c, Item &item) noexcept;
    void pointerEnterEvent() noexcept override;
    void pointerButtonEvent() noexcept override;

    void onTrashDirChange(const QString &) noexcept;
    void setEmpty(bool empty) noexcept;

    bool empty { true };
    QFileSystemWatcher fsWatcher;
    QString trashDir;
};

#endif // TRASHCONTROLLER_H
