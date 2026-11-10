#include <cstdlib>
#include <Core/Shelf.h>

int main()
{
    // Apps launched from here (Qt ones) use the Cuarzo platform theme plugin.
    setenv("QT_QPA_PLATFORMTHEME", "cuarzo", 1);

    Shelf shelf;

    if (!shelf.marco)
        return 1;

    return shelf.marco->run();
}
