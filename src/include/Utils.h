#pragma once

#include <sstream>
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

    inline std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }

        return tokens;
    }

    inline std::vector<std::string> splitString(const std::string& str, const std::string& delimiter = ",", size_t maxSplits = std::string::npos) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = 0;
        size_t splits = 0;

        while ((end = str.find(delimiter, start)) != std::string::npos && splits < maxSplits) {
            tokens.push_back(str.substr(start, end - start));  // Add substring before the delimiter
            start = end + delimiter.length();                  // Move past the delimiter
        }

        tokens.push_back(str.substr(start));  // Add the remaining part after the last delimiter
        return tokens;
    }

    inline std::vector<std::string> splitStringInPlace(std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        char* start = &str[0];  // Pointer to the beginning of the string
        char* end = start;

        while (*end != '\0') {  // Loop until the end of the string
            if (*end == delimiter) {
                *end = '\0';  // Replace the delimiter with a null terminator
                tokens.push_back(start);  // Store the start pointer as a string in the vector
                start = end + 1;  // Move the start pointer to the next character
            }
            end++;
        }

        tokens.push_back(start);  // Add the last token after the loop ends

        return tokens;
    }
}
