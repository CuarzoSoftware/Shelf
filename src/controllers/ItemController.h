#ifndef ITEMCONTROLLER_H
#define ITEMCONTROLLER_H

#include <core/Object.h>

class Item;
class SkCanvas;

class ItemController : public Object
{
public:
    enum Type
    {
        App,
        Dir,
        Thumbnail,
        Trash
    };
    const std::list<Item*> &items() const noexcept { return m_items; }
    sk_sp<SkImage> icon() const noexcept { return m_icon; }

    virtual void drawItem(SkCanvas *c, Item &item) noexcept = 0;
    float scale() const noexcept { return m_scale; }
    void setScale(float scale) noexcept { m_scale = scale; }

protected:
    ItemController(Type type, bool running, const QString &name, const QPixmap &icon, const QString &iconName) noexcept;

private:
    friend class Item;
    float m_scale { 1.f };

    std::list<Item*> m_items;
    Type m_type;
    bool m_running;
    sk_sp<SkImage> m_icon;
    QString m_name;
    QString m_iconName;
};

#endif // ITEMCONTROLLER_H
