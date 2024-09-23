#include <LayerShellQt/Shell>
#include <controller/Shelf.h>

int main(int argc, char **argv)
{
    LayerShellQt::Shell::useLayerShell();
    Shelf app(argc, argv);
    app.exec();
	return 0;
}
