#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <include/core/SkImage.h>
#include <QString>
#include <controllers/ItemController.h>

class AppController final : public ItemController
{
public:
    AppController(bool running, const QString &name, const QString &iconName) noexcept;
    void drawItem(SkCanvas *c, Item &item) noexcept override;
    void pointerEnterEvent() noexcept override;

    sk_sp<SkImage> icon;
    bool running;
    QString iconName;
    QString appId;
};

#endif // APPCONTROLLER_H
