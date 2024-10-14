#ifndef POPUP_H
#define POPUP_H

#include <windows/Window.h>
#include <include/core/SkPoint.h>
#include <functional>
#include <QString>

class Dock;
struct xdg_surface;
struct xdg_popup;

struct MenuItem
{
    QString text;
    bool tick;
    std::function<void(MenuItem *)> onClick;
    SkRect textBounds;
};

class Popup final : public Window
{
public:
    Popup(Dock &dock, const SkPoint &pos, const std::vector<MenuItem> &items) noexcept;
    ~Popup() noexcept;

    bool updateDimensions() noexcept;
    void paintEvent(SkCanvas *c) noexcept override;

    Dock &dock;
    std::vector<MenuItem> items;
    xdg_surface *xdgSurface { nullptr };
    xdg_popup *xdgPopup { nullptr };

    SkSize containerSize;
    SkPoint pos;
    SkIPoint surfacePos;
    float caretX;
};

#endif // POPUP_H
