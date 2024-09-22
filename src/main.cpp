#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#include <QApplication>
#include <QDebug>

#include <Manager.h>
#include <Shelf.h>

int main(int argc, char **argv)
{
    LayerShellQt::Shell::useLayerShell();
    QApplication app(argc, argv);
    Manager::get();
    Shelf shelf;
    app.exec();
	return 0;
}
