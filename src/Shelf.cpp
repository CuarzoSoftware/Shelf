#include <AK/AKLog.h>
#include <Shelf.h>
#include <Theme.h>

Shelf::Shelf() noexcept
{
    setTheme(new Theme());

    if (!wayland().layerShell)
    {
        AKLog::fatal("WLR LAYER SHELL not supported by the current compositor.");
        exit(1);
    }

    onScreenPlugged.subscribe(this, [this](MScreen &screen){
        createDockForScreen(&screen);
    });

    onScreenUnplugged.subscribe(this, [this](MScreen &screen){
        destroyDock(&screen);
    });

    createDocks();
}

void Shelf::createDocks() noexcept
{
    for (MScreen *screen : screens())
        createDockForScreen(screen);
}

Dock *Shelf::createDockForScreen(MScreen *screen) noexcept
{
    const auto it { docks.find(screen) };

    if (it == docks.end())
        return docks.emplace(screen, std::make_unique<Dock>(screen)).first->second.get();

    return it->second.get();
}

void Shelf::destroyDock(MScreen *screen) noexcept
{
    docks.erase(screen);
}
