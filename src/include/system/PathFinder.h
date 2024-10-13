#pragma once

#include <filesystem>
#include <optional>

namespace PathFinder {
    std::optional<std::filesystem::path> home();
    std::optional<std::filesystem::path> liveBundle();
    std::optional<std::filesystem::path> liveBinary();
    std::optional<std::filesystem::path> liveThemes();
    std::optional<std::filesystem::path> configFilePath();
    std::optional<std::filesystem::path> configMenuPath();

    // we're creating the pipes so we can't do an isExists
    // check or return nullopt
    std::filesystem::path requestPipe();
    std::filesystem::path responsePipe();
}
