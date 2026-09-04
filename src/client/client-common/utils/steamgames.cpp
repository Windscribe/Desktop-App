#include "steamgames.h"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

namespace SteamGames {
namespace {

std::string canonicalizeIfExists(const std::string &path)
{
    char *resolved = realpath(path.c_str(), nullptr);
    if (resolved == nullptr) {
        return std::string();
    }
    std::string out(resolved);
    free(resolved);
    return out;
}

bool isDirectory(const std::string &path)
{
    struct stat st = {};
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// VDF files store key/value pairs as `"key"  "value"` on one line. Returns the first
// double-quoted string following the quoted key on this line, or "" when the key is absent.
std::string vdfValueOnLine(const std::string &key, const std::string &line)
{
    const std::string quotedKey = "\"" + key + "\"";
    const size_t keyPos = line.find(quotedKey);
    if (keyPos == std::string::npos) {
        return std::string();
    }
    const size_t valueStart = line.find('"', keyPos + quotedKey.size());
    if (valueStart == std::string::npos) {
        return std::string();
    }
    const size_t valueEnd = line.find('"', valueStart + 1);
    if (valueEnd == std::string::npos) {
        return std::string();
    }
    return line.substr(valueStart + 1, valueEnd - valueStart - 1);
}

// First matching `"key"  "value"` line in the file, or "".
std::string vdfValue(const std::string &key, const std::string &file)
{
    std::ifstream f(file);
    std::string line;
    while (std::getline(f, line)) {
        const std::string value = vdfValueOnLine(key, line);
        if (!value.empty()) {
            return value;
        }
    }
    return std::string();
}

// Extra libraries listed in this root's steamapps/libraryfolders.vdf. Malformed or nonexistent
// entries are skipped; the root itself is commonly listed here too and is deduped by the caller.
std::vector<std::string> libraryPathsForRoot(const std::string &root)
{
    std::vector<std::string> libs;
    std::ifstream f(root + "/steamapps/libraryfolders.vdf");
    std::string line;
    while (std::getline(f, line)) {
        const std::string path = vdfValueOnLine("path", line);
        if (!path.empty() && isDirectory(path + "/steamapps")) {
            libs.push_back(path);
        }
    }
    return libs;
}

// appmanifest_<appid>.acf files carry the install state. "name" is the display name and
// "installdir" the folder under steamapps/common/; the directory's existence is the gate for
// "actually installed" (a half-downloaded update still leaves the directory in place, while an
// uninstalled game removes it but may leave a stale manifest behind).
void enumerateLibrary(const std::string &library, std::vector<Game> &games,
                      std::vector<std::string> &seenDirs)
{
    const std::string steamapps = library + "/steamapps";
    DIR *dp = opendir(steamapps.c_str());
    if (dp == nullptr) {
        return;
    }
    struct dirent *ep;
    while ((ep = readdir(dp))) {
        const std::string name = ep->d_name;
        const size_t prefixLen = std::string("appmanifest_").size();
        if (name.rfind("appmanifest_", 0) != 0 || name.size() <= prefixLen + 1
            || name.compare(name.size() - 4, 4, ".acf") != 0) {
            continue;
        }
        const std::string appId = name.substr(prefixLen, name.size() - 4 - prefixLen);
        if (appId.find_first_not_of("0123456789") != std::string::npos) {
            continue;
        }

        const std::string acf = steamapps + "/" + name;
        const std::string installdir = vdfValue("installdir", acf);
        // Steam's own installdir values are plain relative names; anything absolute or
        // traversing upwards comes from a corrupt or hostile manifest and must not escape the
        // library (it would surface as a directory-prefix split-tunnel entry).
        if (installdir.empty() || installdir.front() == '/'
            || installdir.find("..") != std::string::npos) {
            continue;
        }
        const std::string gameDir = canonicalizeIfExists(steamapps + "/common/" + installdir);
        if (gameDir.empty()) {
            continue;
        }
        if (std::find(seenDirs.begin(), seenDirs.end(), gameDir) != seenDirs.end()) {
            continue;
        }
        seenDirs.push_back(gameDir);

        Game game;
        game.installDir = gameDir;
        game.appId = appId;
        game.name = vdfValue("name", acf);
        if (game.name.empty()) {
            game.name = installdir;
        }
        games.push_back(std::move(game));
    }
    closedir(dp);
}

} // namespace

std::vector<Game> enumerateInstalledGames()
{
    std::vector<Game> games;
    std::vector<std::string> seenDirs;

    const char *home = getenv("HOME");
    if (home == nullptr || *home == '\0') {
        return games;
    }
    const std::string homeDir(home);

    // Seed roots. ~/.steam/steam is a symlink Steam maintains to wherever the client itself is
    // installed (including custom locations on other drives), so it covers relocated installs.
    // Extra *libraries* on other drives are discovered per root via libraryfolders.vdf below.
    std::vector<std::string> roots = {
        homeDir + "/.local/share/Steam",                                  // native default
        homeDir + "/.steam/steam",                                        // native alias (symlink)
        homeDir + "/.steam/debian-installation",                          // native, deb layout
        homeDir + "/.var/app/com.valvesoftware.Steam/.local/share/Steam", // Flatpak Steam
        homeDir + "/snap/steam/common/.local/share/Steam",                // Snap Steam
    };

    // Honor XDG_DATA_HOME when it is set to an absolute path, as ~/.local/share is only the
    // default data home.
    const char *xdgDataHome = getenv("XDG_DATA_HOME");
    if (xdgDataHome != nullptr && xdgDataHome[0] == '/') {
        roots.push_back(std::string(xdgDataHome) + "/Steam");
    }

    std::vector<std::string> libraries;
    const auto addLibrary = [&libraries](const std::string &path) {
        if (!path.empty() && std::find(libraries.begin(), libraries.end(), path) == libraries.end()) {
            libraries.push_back(path);
        }
    };
    for (const auto &root : roots) {
        const std::string canonicalRoot = canonicalizeIfExists(root);
        if (canonicalRoot.empty() || !isDirectory(canonicalRoot + "/steamapps")) {
            continue;
        }
        addLibrary(canonicalRoot);
        // Libraries registered for this root; these are how additional drives/locations are
        // known (e.g. /mnt/games/SteamLibrary). Deduped on their canonical path so a library
        // reachable through several spellings (symlink, bind mount alias) is scanned once.
        for (const auto &lib : libraryPathsForRoot(canonicalRoot)) {
            addLibrary(canonicalizeIfExists(lib));
        }
    }

    for (const auto &library : libraries) {
        enumerateLibrary(library, games, seenDirs);
    }
    return games;
}

} // namespace SteamGames
