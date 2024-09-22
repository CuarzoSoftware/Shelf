#include <Shelf.h>
#include <Dock.h>

Dock::Dock(Shelf *shelf) :
    QFrame(shelf),
    shelf(shelf)
{
    setStyleSheet(".QFrame{background-color: rgba(250,250,250,0.9); border-radius: 20px;}");
    shadowFx->setBlurRadius(64);
    shadowFx->setOffset(0, 0);
    setGraphicsEffect(shadowFx);
}
