#include <Core/Shelf.h>

int main()
{
    Shelf shelf;

    if (!shelf.marco)
        return 1;

    return shelf.marco->run();
}
