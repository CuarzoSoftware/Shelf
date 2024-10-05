//#include <LayerShellQt/Shell>
//#include <controller/Shelf.h>

#include <QApplication>
#include <core/Shelf.h>

int main(int argc, char **argv)
{
    /* LayerShellQt::Shell::useLayerShell();
    Shelf app(argc, argv);
    app.exec(); */

    QApplication app { argc, argv };
    return Shelf::run();
}
