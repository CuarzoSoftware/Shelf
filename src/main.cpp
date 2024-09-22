#include <LayerShellQt/Shell>
#include <Application.h>
#include <Global.h>
#include <Shelf.h>

int main(int argc, char **argv)
{
    LayerShellQt::Shell::useLayerShell();
    Application app(argc, argv);
    Global::get();
    Shelf shelf;
    app.exec();
	return 0;
}
