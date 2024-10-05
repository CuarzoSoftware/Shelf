//#include <LayerShellQt/Shell>
//#include <controller/Shelf.h>

#include <QGuiApplication>
#include <Shelf.h>

int main(int argc, char **argv)
{
    /* LayerShellQt::Shell::useLayerShell();
    Shelf app(argc, argv);
    app.exec(); */

    QGuiApplication app { argc, argv };
    return Shelf::run();
}
