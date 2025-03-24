#ifndef APPLICATION_H
#define APPLICATION_H

#include <Marco/MApplication.h>
#include <Theme.h>
#include <Dock.h>

using namespace AK;

class Shelf : public MApplication
{
public:
    Shelf() noexcept;
    void createDocks() noexcept;
    Dock *createDockForScreen(MScreen *screen) noexcept;
    void destroyDock(MScreen *screen) noexcept;
    std::unordered_map<MScreen*, std::unique_ptr<Dock>> docks;

    static Theme &theme() noexcept { return *(Theme*)AK::theme(); };
};

#endif // APPLICATION_H
