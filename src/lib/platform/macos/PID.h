#ifndef PROCESS_H
#define PROCESS_H

#include <string>

class LogHandler;

class PID {
public:
    static PID& getInstance();

    PID(const PID&) = delete;
    PID& operator=(const PID&) = delete;

    pid_t findByName(std::string processName);
    pid_t livePID();
    pid_t appPID();

    PID* init();

private:
    PID();
    ~PID();

    LogHandler* log_;

    pid_t abletonLivePID = -1;
};

#endif
