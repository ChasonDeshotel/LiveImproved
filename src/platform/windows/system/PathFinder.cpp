#include <Windows.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <filesystem>
#include <regex>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

namespace PathFinder {
    fs::path home() {
        PWSTR path = nullptr;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);
        
        if (SUCCEEDED(result)) {
            std::filesystem::path homePath(path);
            CoTaskMemFree(path);
            return homePath;
        }
        
        CoTaskMemFree(path);  // Free memory even if the function failed
        throw std::runtime_error("Failed to get user's home directory");
    }

    fs::path config() {
        return fs::path(documents()) / "Ableton" / "User Library" / "LiveImproved" / "config.txt";
    }

    fs::path configMenu() {
        return fs::path(documents()) / "Ableton" / "User Library" / "LiveImproved" / "config-menu.txt";
        throw std::runtime_error("Failed to get config menu file path");
    }

    fs::path documents() {
        PWSTR path = nullptr;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);
        
        if (SUCCEEDED(result)) {
            std::filesystem::path docPath(path);
            CoTaskMemFree(path);
            return docPath;
        }
        
        CoTaskMemFree(path);
        throw std::runtime_error("Failed to get user's Documents directory");
    }

    fs::path localAppData() {
        PWSTR path = nullptr;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path);
        
        if (SUCCEEDED(result)) {
            fs::path appDataPath(path);
            CoTaskMemFree(path);
            return appDataPath;
        }
        
        CoTaskMemFree(path);  // Free memory even if the function failed
        throw std::runtime_error("Failed to get user's AppData directory");
    }

    fs::path log() {
        fs::path(localAppData()) / "Logs" / "YourAppName.log";
        throw std::runtime_error("Failed to get log path");
    }

    fs::path liveBundle() {
        return fs::path(localAppData()) / "Ableton" / "Live";
        //throw std::runtime_error("Failed to get Ableton Live bundle path");
    }

    fs::path liveBinary() {
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

    fs::path getProcessPath(DWORD processId) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess == NULL) {
            throw std::system_error(GetLastError(), std::system_category(), "OpenProcess failed");
        }

        wchar_t buffer[MAX_PATH];
        DWORD result = GetModuleFileNameExW(hProcess, NULL, buffer, MAX_PATH);
        
        CloseHandle(hProcess);

        if (result == 0) {
            throw std::system_error(GetLastError(), std::system_category(), "GetModuleFileNameEx failed");
        }

        return std::filesystem::path(buffer);
    }

    fs::path liveTheme() {
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Ableton\\Live", 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open Ableton Live registry key");
        }

        char version[256];
        DWORD bufSize = sizeof(version);
        result = RegQueryValueExA(hKey, "Version", NULL, NULL, (LPBYTE)version, &bufSize);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to query Ableton Live version");
        }

        std::string versionStr(version);
        std::regex versionRegex(R"(\d+\.\d+)");
        std::smatch match;
        if (!std::regex_search(versionStr, match, versionRegex)) {
            throw std::runtime_error("Failed to parse Ableton Live version");
        }

        fs::path themePath = "C:\\ProgramData\\Ableton\\Live " + match.str() + "\\Themes";
        if (!fs::exists(themePath)) {
            throw std::runtime_error("Ableton Live Themes folder not found");
        }

        return themePath;
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
