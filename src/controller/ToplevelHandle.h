#ifndef TOPLEVELHANDLE_H
#define TOPLEVELHANDLE_H

#include <QString>
#include <list>

class AppController;
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
    std::list<ToplevelHandle*>::iterator shelfLink;
    zwlr_foreign_toplevel_handle_v1 *handle;

    std::list<ToplevelHandle*>::iterator appControllerLink;
    AppController *appController { nullptr };
    // ItemController *thumbnailController { nullptr };
};

#endif // TOPLEVELHANDLE_H
