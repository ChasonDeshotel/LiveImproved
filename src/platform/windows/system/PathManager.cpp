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

#include "PathManager.h"

namespace fs = std::filesystem;
using Path = std::filesystem::path;

PathManager::PathManager() {}

// accounts for symlinks
auto PathManager::isValidFile(const fs::path& path) const -> bool {
    fs::file_status status = fs::symlink_status(path);
    return fs::is_regular_file(status);
}

// accounts for symlinks
auto PathManager::isValidDir(const fs::path& path) const -> bool {
    fs::file_status status = fs::symlink_status(path);
    return fs::is_directory(status);
}

auto PathManager::home() const -> Path {
    PWSTR path = nullptr;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);

    if (SUCCEEDED(result)) {
        Path homePath(path);
        CoTaskMemFree(path);
        return homePath;
    }

    CoTaskMemFree(path);  // Free memory even if the function failed
    throw std::runtime_error("Failed to get user's home directory");
}

auto PathManager::config() const -> Path {
    Path configFile = limRemoteScript() / "config.txt";

    if (!isValidFile(configFile)) {
        throw std::runtime_error("Config doesn't exist or is not a file!");
    }

    return configFile;
}

auto PathManager::configMenu() const -> Path {
    Path configFile = limRemoteScript() / "config-menu.txt";

    if (!isValidFile(configFile)) {
        throw std::runtime_error("Menu config doesn't exist or is not a file!");
    }

    return configFile;
}

auto PathManager::remoteScripts() const -> Path {
    Path remoteScripts = documents() / "Ableton" / "User Library" / "Remote Scripts";


    if (!isValidDir(remoteScripts)) {
        throw std::runtime_error("Ableton Live MIDI Remote Scripts directory does not exist or is not a directory: " + remoteScripts.string());
    }

    return remoteScripts;
}

auto PathManager::limRemoteScript() const -> Path {
    Path limRemoteScript = remoteScripts() / "LiveImproved";

    if (!isValidDir(limRemoteScript)) {
        throw std::runtime_error("LiveImproved MIDI Remote Script directory does not exist or is not a directory: " + limRemoteScript.string());
    }

    return limRemoteScript;
}

auto PathManager::lesConfig() const -> Path {
    Path lesConfig = home() / ".les" / "menuconfig.ini";

    if (!isValidFile(lesConfig)) {
        throw std::runtime_error("Live Enhancement Suite config file is not found or is invalid: " + lesConfig.string());
    }

    return lesConfig;
}

auto PathManager::documents() const -> Path {
    PWSTR path = nullptr;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

    if (SUCCEEDED(result)) {
        Path docPath(path);
        CoTaskMemFree(path);
        return docPath;
    }

    CoTaskMemFree(path);
    throw std::runtime_error("Failed to get user's Documents directory");
}

auto PathManager::localAppData() const -> Path {
    PWSTR path = nullptr;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path);

    if (SUCCEEDED(result)) {
        Path appDataPath(path);
        CoTaskMemFree(path);
        return appDataPath;
    }

    CoTaskMemFree(path);  // Free memory even if the function failed
    throw std::runtime_error("Failed to get user's AppData directory");
}

auto PathManager::log() const -> Path {
    Path log = localAppData() / "LiveImproved" / "log.txt";
    throw std::runtime_error("Failed to get log path");
    return log;
}

auto PathManager::liveBundle() const -> Path {
    return fs::path(localAppData()) / "Ableton" / "Live";
    //throw std::runtime_error("Failed to get Ableton Live bundle path");
}

auto PathManager::liveBinary() const -> Path {
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

//auto PathManager::getProcessPath(DWORD processId) -> Path {
//    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
//    if (hProcess == NULL) {
//        throw std::system_error(GetLastError(), std::system_category(), "OpenProcess failed");
//    }
//
//    wchar_t buffer[MAX_PATH];
//    DWORD result = GetModuleFileNameExW(hProcess, NULL, buffer, MAX_PATH);
//
//    CloseHandle(hProcess);
//
//    if (result == 0) {
//        throw std::system_error(GetLastError(), std::system_category(), "GetModuleFileNameEx failed");
//    }
//
//    return std::filesystem::path(buffer);
//}

auto PathManager::liveTheme() const -> Path {
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

//auto PathManager::findExecutable(const std::string& name) -> Path {
//    char buffer[MAX_PATH];
//    std::string path = getEnvVar("PATH");
//    std::vector<std::string> paths = splitPath(path);
//
//    for (const auto& dir : paths) {
//        std::string fullPath = dir + "\\" + name;
//        if (PathFileExistsA(fullPath.c_str())) {
//            if (GetFullPathNameA(fullPath.c_str(), MAX_PATH, buffer, nullptr) != 0) {
//                return std::string(buffer);
//            }
//        }
//    }
//
//    throw std::runtime_error("Executable not found: " + name);
//}

auto PathManager::requestPipe() const -> Path {
    return limRemoteScript() / "lim_request";
}

auto PathManager::responsePipe() const -> Path {
    return limRemoteScript() / "lim_response";
}
