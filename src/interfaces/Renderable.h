#ifndef RENDERABLE_H
#define RENDERABLE_H

class SkCanvas;

class Renderable
{
public:
    virtual void paintEvent(SkCanvas *c) noexcept = 0;
};

#endif // RENDERABLE_H
