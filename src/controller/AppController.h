#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <controller/ItemController.h>
#include <list>

class ToplevelHandle;

class AppController final : public ItemController
{
public:
    AppController(const QString &iconName, const QString &name, const QString &id, const QString &desktopFilePath, bool pinned, bool active) noexcept;
    ~AppController() noexcept;
    bool isPinnable() const noexcept;
    void addToplevelHandle(ToplevelHandle *handle) noexcept;
    void removeToplevelHandle(ToplevelHandle *handle) noexcept;
    const std::list<ToplevelHandle*> &toplevelHandles() const noexcept;

    const QString &id() const noexcept;

private:
    std::list<AppController*>::iterator shelfLink;
    std::list<ToplevelHandle*> m_toplevelHandles;
    QString m_id;
    QString m_desktopFilePath;
    bool m_pinned;
};

#endif // APPCONTROLLER_H
