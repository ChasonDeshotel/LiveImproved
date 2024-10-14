#pragma once

#include <string>
#include "Types.h"

class PID {
public:
    static auto getInstance() -> PID&;

    PID(const PID&) = delete;
    auto operator=(const PID&) -> PID& = delete;

    auto findLivePID() -> pid_t;
    auto livePID() -> pid_t;
    auto appPID() -> pid_t;

    auto livePIDBlocking() -> PID*;

private:
    PID();
    ~PID();

    pid_t abletonLivePID = -1;
};
