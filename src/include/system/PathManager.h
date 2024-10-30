#pragma once

#include <filesystem>
#include <vector>
#include <unordered_map>
#include <string>

using Path = std::filesystem::path;
using PathOptional = std::optional<std::filesystem::path>;

class PathManager {
public:
    PathManager();

    auto log() const -> Path;

    auto home() const -> Path;
    auto documents() const -> Path;

    auto liveBundle() const -> Path;
    auto liveBinary() const -> Path;
    auto liveThemes() const -> Path;
    auto liveTheme() const -> Path;

    auto config() const -> Path;
    auto configMenu() const -> Path;

    auto requestPipe() const -> Path;
    auto responsePipe() const -> Path;

    auto remoteScripts() const -> Path;
    auto limRemoteScript() const -> Path;

    auto lesConfig() const -> Path;

    void addSearchPath(const Path& path);
    void setFallbackPath(const std::string& key, const Path& path);

    #ifdef _WIN32
    auto localAppData() const -> Path;
    #endif

protected:
    auto isValidDir(const Path& dir) const -> bool;
    auto isValidFile(const Path& file) const -> bool;

    std::vector<Path> searchPaths;
    std::unordered_map<std::string, Path> fallbackPaths;
    mutable std::unordered_map<std::string, Path> cachedPaths;

    auto findPath(const std::string& key) const -> Path;
};
