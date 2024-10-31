#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream>
#include <string>

#include "LogHandler.h"
#include "PID.h"

PID::PID()
    : abletonLivePID(-1)
{}

PID::~PID() {}

PID& PID::getInstance() {
    static PID instance;
    return instance;
}

pid_t PID::findLivePID() {
    // LogHandler::getInstance().info("PID::findWithToolHelp() called");

    if (abletonLivePID != -1) {
        // LogHandler::getInstance().info("PID::findByName() - returning cached result");
        return abletonLivePID;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogHandler::getInstance().error("Failed to create process snapshot");
        return -1;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        LogHandler::getInstance().error("Failed to retrieve first process");
        CloseHandle(hSnapshot);
        return -1;
    }

    do {
        // TODO: Check for other versions of Ableton Live
        //if (std::string(pe32.szExeFile) == "Ableton Live 11 Suite.exe") {
        if (std::string(pe32.szExeFile) == "Ableton Live 12 Trial.exe") {
            pid_t pid = pe32.th32ProcessID;
            CloseHandle(hSnapshot);
            LogHandler::getInstance().info("Ableton Live found with PID: " + std::to_string(pid));
            abletonLivePID = pid;
            return pid;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    LogHandler::getInstance().error("Ableton Live not found");
    return -1;
}

pid_t PID::livePID() {
    // LogHandler::getInstance().info("livePID() called");
    if (abletonLivePID != -1) {
        // LogHandler::getInstance().info("PID::livePID() - returning cached result");
        return abletonLivePID;
    }

    return findLivePID();
}

pid_t PID::appPID() {
    return GetCurrentProcessId();
}

PID* PID::livePIDBlocking() {
    LogHandler::getInstance().debug("PID::Init() called");

    while (livePID() == -1) {
        LogHandler::getInstance().info("Live not found, retrying...");
        livePID();
        Sleep(1000); // 1s
    }

    return this;
}
