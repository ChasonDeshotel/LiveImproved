#pragma once

#include <filesystem>
#include <optional>

namespace PathFinder {
    auto home() -> std::optional<std::filesystem::path>;
    auto log() -> std::optional<std::filesystem::path>;
    auto liveBundle() -> std::optional<std::filesystem::path>;
    auto liveBinary() -> std::optional<std::filesystem::path>;
    auto liveThemes() -> std::optional<std::filesystem::path>;
    auto liveTheme() -> std::optional<std::filesystem::path>;
    auto config() -> std::optional<std::filesystem::path>;
    auto configMenu() -> std::optional<std::filesystem::path>;

    // we're creating the pipes so we can't do an isExists
    // check or return nullopt
    auto requestPipe() -> std::filesystem::path;
    auto responsePipe() -> std::filesystem::path;
}
