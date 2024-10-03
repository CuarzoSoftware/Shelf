#ifndef TOPLEVELHANDLE_H
#define TOPLEVELHANDLE_H

#include <QString>
#include <list>
#include <cstdint>

class AppController;
class ThumbnailController;
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
    ThumbnailController *thumbnailController { nullptr };

    uint32_t pendingState {0};
    uint32_t currentState {0};

    void sendActivate() noexcept;
    void sendMinimized(bool minimized) noexcept;
};

#endif // TOPLEVELHANDLE_H
