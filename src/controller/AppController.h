#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QTimer>
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
    const QString &idName() const noexcept;
    void onItemClick(DockItem *item) noexcept override;

private:
    QTimer *launchTimeout { new QTimer(this) };
    std::list<AppController*>::iterator shelfLink;
    std::list<ToplevelHandle*> m_toplevelHandles;
    QString m_id, m_idName;
    QString m_desktopFilePath;
    bool m_pinned;
};

#endif // APPCONTROLLER_H
