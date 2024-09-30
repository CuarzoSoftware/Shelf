#include <controller/TrashController.h>
#include <controller/Shelf.h>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

TrashController::TrashController() noexcept :
    ItemController(Trash, "Trash", QPixmap(), false)
{
    m_emptyIcon = Shelf::iconFromName("user-trash", 512);
    m_fullIcon = Shelf::iconFromName("user-trash-full", 512);
    setIcon(m_emptyIcon);
    Shelf::get().trash = this;

    m_trashDir = QDir::home().filePath(".local/share/Trash/files");
    m_watcher.addPath(m_trashDir);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &TrashController::onTrashDirChange);
    onTrashDirChange(m_trashDir);
}

void TrashController::onTrashDirChange(const QString &) noexcept
{
    auto files = QDir(m_trashDir).entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    qInfo() << files;
    setEmpty(files.isEmpty());
}

void TrashController::setEmpty(bool empty) noexcept
{
    if (empty == m_empty)
        return;

    m_empty = empty;
    setIcon(empty ? m_emptyIcon : m_fullIcon);
}

void TrashController::onItemClick(DockItem *item) noexcept
{
    QDesktopServices::openUrl(QUrl(m_trashDir));
}
