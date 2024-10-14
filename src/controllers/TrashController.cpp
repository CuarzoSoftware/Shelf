#include <controllers/TrashController.h>
#include <include/core/SkCanvas.h>
#include <include/effects/SkImageFilters.h>
#include <items/Item.h>
#include <QPixmap>
#include <windows/Dock.h>
#include <core/Utils.h>
#include <core/Constants.h>
#include <core/Shelf.h>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <linux/input-event-codes.h>

TrashController::TrashController() noexcept : ItemController(Trash, "Trash")
{
    trashDir = QDir::home().filePath(".local/share/Trash/files");
    fsWatcher.addPath(trashDir);
    QObject::connect(&fsWatcher, &QFileSystemWatcher::directoryChanged, this, &TrashController::onTrashDirChange);
    onTrashDirChange(trashDir);
}

void TrashController::drawItem(SkCanvas *c, Item &item) noexcept
{
    c->restore();
    static const SkSamplingOptions linear { SkFilterMode::kLinear };
    const float size { item.dock.itemSize * scale };
    const float iconSpaceH { item.dock.rect.height() - item.dock.dotMarginB - item.dock.dotMarginT - item.dock.dotSize };
    const SkRect rect { SkRect::MakeXYWH(
        item.rect.x() + (item.rect.width() - size) / 2.f,
        item.dock.rect.y() + (iconSpaceH - size) / 2.f,
        size, size) };

    sk_sp<SkImageFilter> blurFilter = SkImageFilters::DropShadow(0.f, 1.f,
                                                                 0.5f, 1.f,
                                                                 SkColorSetARGB(64, 0, 0, 0), nullptr);

    SkPaint p;
    p.setAntiAlias(false);
    p.setImageFilter(blurFilter);

    if (&item == shelf().draggedItem)
        p.setColorFilter(shelf().darkenFilter);

    c->drawImageRect(
        empty ? shelf().trashEmptyIcon.pick(item.dock.itemSize) : shelf().trashFullIcon.pick(item.dock.itemSize),
        rect, linear, &p);
}

void TrashController::pointerEnterEvent() noexcept
{
    Item &item { *shelf().focusedItem };
    Tooltip &tooltip { shelf().focusedItem->dock.tooltip };
    tooltip.showItem(&item);
}

void TrashController::pointerButtonEvent() noexcept
{
    if (shelf().pointerButton == BTN_LEFT && shelf().pointerButtonState == WL_POINTER_BUTTON_STATE_RELEASED)
        QDesktopServices::openUrl(QUrl(trashDir));

    shelf().focusedItem->dock.update();
}

void TrashController::onTrashDirChange(const QString &) noexcept
{
    auto files = QDir(trashDir).entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    setEmpty(files.isEmpty());
}

void TrashController::setEmpty(bool empty) noexcept
{
    if (this->empty == empty)
        return;

    this->empty = empty;

    for (Item *item : items)
        item->dock.update();
}
