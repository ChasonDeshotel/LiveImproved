#pragma once

#include <string>
#include "Types.h"

class PID {
public:
    static PID& getInstance();

    PID(const PID&) = delete;
    PID& operator=(const PID&) = delete;

    pid_t findLivePID();
    pid_t livePID();
    pid_t appPID();

    PID* livePIDBlocking();

private:
    PID();
    ~PID();

    pid_t abletonLivePID = -1;
};
