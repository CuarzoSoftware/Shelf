#ifndef POINTEREVENTSLISTENER_H
#define POINTEREVENTSLISTENER_H

class PointerEventsListener
{
public:
    virtual void pointerEnterEvent() noexcept {}
    virtual void pointerLeaveEvent() noexcept {}
    virtual void pointerMotionEvent() noexcept {}
    virtual void pointerButtonEvent() noexcept {}
};

#endif // POINTEREVENTSLISTENER_H
