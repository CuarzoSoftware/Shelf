#include <Shelf.h>

int main (void)
{
    setenv("KAY_DEBUG", "4", 0);
    Shelf shelf;
    return shelf.exec();
}
