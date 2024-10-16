#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <tlhelp32.h>

namespace fs = std::filesystem;

namespace PathFinder {
    std::string home() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
            return std::string(path);
        }
        throw std::runtime_error("Failed to get home directory");
    }

    std::string log() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            return fs::path(path) / "Logs" / "YourAppName.log";
        }
        throw std::runtime_error("Failed to get log path");
    }

    std::string liveBundle() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            return fs::path(path) / "Ableton" / "Live";
        }
        throw std::runtime_error("Failed to get Ableton Live bundle path");
    }

    std::string liveBinary() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create process snapshot");
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            throw std::runtime_error("Failed to get first process");
        }

        do {
            if (std::string(pe32.szExeFile).find("Ableton Live") != std::string::npos) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    char path[MAX_PATH];
                    if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH) != 0) {
                        CloseHandle(hProcess);
                        CloseHandle(hSnapshot);
                        return std::string(path);
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        throw std::runtime_error("Ableton Live process not found");
    }

    std::string findExecutable(const std::string& name) {
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
