#ifndef ITEMCONTROLLER_H
#define ITEMCONTROLLER_H

#include <QPropertyAnimation>
#include <QPixmap>
#include <QObject>

class DockItem;

class ItemController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double jumpPhase READ jumpPhase WRITE setJumpPhase NOTIFY jumpPhaseChanged)
public:
    enum Type
    {
        App,
        Dir,
        Thumbnail,
        Trash
    };

    ItemController(Type type, const QString &name, const QPixmap &icon, bool running, const QString &iconName) noexcept;
    virtual ~ItemController() noexcept;

    std::list<DockItem*> &items() noexcept;

    const QString &name() const noexcept;
    void setName(const QString &name) noexcept;

    const QString &iconName() const noexcept;

    const QPixmap &icon() const noexcept;
    void setIcon(const QPixmap &icon) noexcept;

    void setIconsJumping(bool jumping) noexcept;
    bool iconsJumping() const noexcept;

    void setJumpPhase(double phase) noexcept;
    double jumpPhase() const noexcept;

    bool running() const noexcept;
    void setRunning(bool running) noexcept;

    Type type() const noexcept;

    virtual void onItemClick(DockItem *item) noexcept = 0;

Q_SIGNALS:
    void jumpPhaseChanged();

private:
    QPropertyAnimation *m_jumpAnim { new QPropertyAnimation(this, "jumpPhase", this) };
    std::list<DockItem*> m_items;
    QPixmap m_icon;
    QString m_name, m_iconName;
    Type m_type;
    bool m_running;
    bool m_iconsJumping { false };
    double m_jumpPhase { 0.0 };
};

#endif // ITEMCONTROLLER_H
