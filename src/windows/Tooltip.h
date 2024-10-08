#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <windows/Window.h>
#include <QString>

class Dock;
class Item;
struct wl_subsurface;

class Tooltip final : public Window
{
public:
    Tooltip(Dock &dock) noexcept;
    ~Tooltip() noexcept;

    void beforePaintEvent() noexcept override;
    void afterPaintEvent() noexcept override;

    void paintEvent(SkCanvas *c) noexcept override;
    bool updateDimensions() noexcept;

    void show(float x, float y, const QString &text) noexcept;
    void showItem(Item *item) noexcept;

    wl_subsurface *wlSubsurface { nullptr };
    Dock &dock;
    QString text;

    SkRect textBounds;
    SkSize containerSize;
    float caretX;

    SkPoint pos;
    SkIPoint surfacePos;
};

#endif // TOOLTIP_H
