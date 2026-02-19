#pragma once
#include <vector>

class PID {
public:
    static auto getInstance() -> PID&;

    PID(const PID&) = delete;
    PID(PID&&) noexcept = delete;
    auto operator=(const PID&) -> PID& = delete;
    auto operator=(PID&&) noexcept -> PID& = delete;

    auto findLiveImproveds() -> std::vector<pid_t>;
    bool isLiveRunning();
    bool isLIMRunning();
    auto findLivePID() -> pid_t;
    auto findLivePIDNoCache() -> pid_t;
    auto livePID() -> pid_t;
    auto appPID() -> pid_t;
    void clearLivePID();
    auto livePIDBlocking() -> PID*;

private:
    PID();
    ~PID();

    pid_t abletonLivePID_ = -1;
};
