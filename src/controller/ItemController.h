#ifndef ITEMCONTROLLER_H
#define ITEMCONTROLLER_H

#include <QPixmap>
#include <QObject>

class DockItem;

class ItemController : public QObject
{
public:
    enum Type
    {
        App,
        Dir,
        Thumbnail,
        Trash
    };

    ItemController(Type type, const QString &name, const QPixmap &icon, bool active) noexcept;
    virtual ~ItemController() noexcept;

    std::list<DockItem*> &items() noexcept;

    const QString &name() const noexcept;
    void setName(const QString &name) noexcept;

    const QPixmap &icon() const noexcept;
    void setIcon(const QPixmap &icon) noexcept;

    bool active() const noexcept;
    void setActive(bool active) noexcept;

    Type type() const noexcept;

private:
    std::list<DockItem*> m_items;
    QPixmap m_icon;
    QString m_name;
    Type m_type;
    bool m_active;
};

#endif // ITEMCONTROLLER_H
