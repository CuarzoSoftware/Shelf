#include <Core/Settings.h>
#include <Core/Log.h>
#include <CZ/XDG/XDGINI.h>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

fs::path Settings::FilePath() noexcept
{
    fs::path dir;

    if (const char *config { getenv("XDG_CONFIG_HOME") }; config && config[0] == '/')
        dir = config;
    else if (const char *home { getenv("HOME") }; home && home[0] == '/')
        dir = fs::path(home) / ".config";
    else if (const passwd *pw { getpwuid(getuid()) }; pw && pw->pw_dir)
        dir = fs::path(pw->pw_dir) / ".config";

    return dir / "cz-shelf" / "shelf.conf";
}

void Settings::load() noexcept
{
    pinnedApps.clear();

    const fs::path path { FilePath() };
    std::error_code ec;

    if (!fs::is_regular_file(path, ec))
    {
        Log(CZInfo, CZLN, "No settings file at {}, using defaults", path.string());
        return;
    }

    auto ini { XDGINI::LoadFile(path) };
    auto dock { ini->find("Dock") };

    if (dock == ini->end())
        return;

    if (auto it { dock->second.find("PinnedApps") }; it != dock->second.end())
    {
        std::string id;

        for (const char c : it->second + ";")
        {
            if (c != ';') { id += c; continue; }
            if (!id.empty() && std::find(pinnedApps.begin(), pinnedApps.end(), id) == pinnedApps.end())
                pinnedApps.emplace_back(id);
            id.clear();
        }
    }

    Log(CZInfo, CZLN, "Loaded {} pinned app(s) from {}", pinnedApps.size(), path.string());
}

void Settings::save() noexcept
{
    const fs::path path { FilePath() };
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    // Write to a temporary file first so a crash can't leave a truncated settings file.
    const fs::path tmp { path.string() + ".tmp" };
    {
        std::ofstream out { tmp, std::ios::trunc };

        if (!out)
        {
            Log(CZError, CZLN, "Failed to write the settings file {}", tmp.string());
            return;
        }

        out << "[Dock]\nPinnedApps=";

        for (const auto &id : pinnedApps)
            out << id << ';';

        out << '\n';
    }

    fs::rename(tmp, path, ec);

    if (ec)
        Log(CZError, CZLN, "Failed to save the settings file {}: {}", path.string(), ec.message());
}
