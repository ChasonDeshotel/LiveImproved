#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <stdexcept>

class PathFinder {
public:
    static std::string findExecutable(const std::string& name) {
        char buffer[MAX_PATH];
        std::string path = getEnvVar("PATH");
        std::vector<std::string> paths = splitPath(path);

        for (const auto& dir : paths) {
            std::string fullPath = dir + "\\" + name;
            if (PathFileExistsA(fullPath.c_str())) {
                if (GetFullPathNameA(fullPath.c_str(), MAX_PATH, buffer, nullptr) != 0) {
                    return std::string(buffer);
                }
            }
        }

        throw std::runtime_error("Executable not found: " + name);
    }

private:
    static std::string getEnvVar(const std::string& name) {
        char* buf = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buf, &sz, name.c_str()) == 0 && buf != nullptr) {
            std::string ret(buf);
            free(buf);
            return ret;
        }
        return "";
    }

    static std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> result;
        char* context = nullptr;
        char* token = strtok_s(const_cast<char*>(path.c_str()), ";", &context);
        while (token != nullptr) {
            result.push_back(token);
            token = strtok_s(nullptr, ";", &context);
        }
        return result;
    }
};
