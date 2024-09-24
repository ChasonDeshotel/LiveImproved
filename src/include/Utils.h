#pragma once

#include <filesystem>

namespace Utils {
    static std::filesystem::path getHomeDirectory() {
        #ifdef _WIN32
            const char* homeDir = getenv("USERPROFILE");
        #else
            const char* homeDir = getenv("HOME");
        #endif

        if (!homeDir) {
            throw std::runtime_error("Could not find the home directory.");
        }

        return std::filesystem::path(homeDir);
    }
}
