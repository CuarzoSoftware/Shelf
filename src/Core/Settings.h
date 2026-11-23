#ifndef SETTINGS_H
#define SETTINGS_H

#include <Core/Types.h>
#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Persistent Shelf settings, stored in `$XDG_CONFIG_HOME/cz-shelf/shelf.conf`
 *        (`~/.config/cz-shelf/shelf.conf` by default).
 *
 * @code
 * [Dock]
 * PinnedApps=org.gnome.Nautilus;firefox;
 * AutoHide=true
 * TileSize=48
 * @endcode
 */
class Settings
{
public:
    /// Loads the settings file (missing file = defaults).
    void load() noexcept;

    /// Writes the settings file, creating its directory if needed.
    void save() noexcept;

    /// App ids pinned to the dock, in dock order.
    std::vector<std::string> pinnedApps;

    /// Whether load() found no settings file (the first launch): see Shelf::pinDefaultApps().
    bool firstLaunch { false };

    /// Whether the dock hides when the pointer isn't over it.
    bool autoHide { true };

    /// Dock icon size (see DockMetrics), set by dragging the divider.
    Int32 tileSize { DOCK_TILE_SIZE_DEFAULT };

    static std::filesystem::path FilePath() noexcept;
};

#endif // SETTINGS_H
