#pragma once

#include <string>
#include <vector>

// Enumeration of installed Steam games, resolved to their on-disk install directories.
// Pure standard C++ (no Qt) so it is usable from both the Qt GUI and non-Qt build contexts,
// and testable standalone against a fixture HOME.
namespace SteamGames {

struct Game
{
    std::string installDir;  // absolute, symlink-resolved path to steamapps/common/<installdir>
    std::string name;        // game name from the app manifest (falls back to installdir)
    std::string appId;       // decimal app id from the manifest filename
};

// Enumerates installed Steam games across all discoverable Steam installations and libraries:
// native roots (~/.local/share/Steam, ~/.steam/steam, ~/.steam/debian-installation), the Flatpak
// Steam root (~/.var/app/com.valvesoftware.Steam/...), plus every extra library listed in each
// root's steamapps/libraryfolders.vdf. Only games whose install directory exists on disk are
// returned; each directory is reported once even when reachable via several roots.
std::vector<Game> enumerateInstalledGames();

} // namespace SteamGames
