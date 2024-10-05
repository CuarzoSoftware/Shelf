#ifndef TRASHCONTROLLER_H
#define TRASHCONTROLLER_H

#include <controller/ItemController.h>
#include <QFileSystemWatcher>

class TrashController final : public ItemController
{
public:
    TrashController() noexcept;

    void onTrashDirChange(const QString &) noexcept;
    void setEmpty(bool empty) noexcept;

    void onItemClick(DockItem *item) noexcept override;

private:
    QString m_trashDir;
    QFileSystemWatcher m_watcher;
    QPixmap m_emptyIcon;
    QPixmap m_fullIcon;
    bool m_empty { true };
};

#endif // TRASHCONTROLLER_H
