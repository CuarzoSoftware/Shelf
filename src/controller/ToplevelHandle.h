#ifndef TOPLEVELHANDLE_H
#define TOPLEVELHANDLE_H

#include <QString>
#include <list>

class ItemController;
struct zwlr_foreign_toplevel_handle_v1;

class ToplevelHandle
{
public:
    ToplevelHandle(zwlr_foreign_toplevel_handle_v1 *handle) noexcept;
    ~ToplevelHandle() noexcept;

    QString currentTitle;
    QString currentAppId;
    QString pendingTitle;
    QString pendingAppId;
    std::list<ToplevelHandle*>::iterator link;
    zwlr_foreign_toplevel_handle_v1 *handle;

    ItemController *appController { nullptr };
    ItemController *thumbnailController { nullptr };
};

#endif // TOPLEVELHANDLE_H
