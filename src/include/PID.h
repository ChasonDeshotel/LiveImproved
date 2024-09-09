#ifndef PROCESS_H
#define PROCESS_H

#include <string>

class LogHandler;

class PID {
public:
    static PID& getInstance();

    PID(const PID&) = delete;
    PID& operator=(const PID&) = delete;

    pid_t findWithSysctl();
    pid_t livePID();
    pid_t appPID();

    PID* livePIDBlocking();

private:
    PID();
    ~PID();

    LogHandler* log_;

    pid_t abletonLivePID = -1;
};

#endif
