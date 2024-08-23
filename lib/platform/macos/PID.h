#ifndef PROCESS_H
#define PROCESS_H

#include <string>

class ApplicationManager;
class LogHandler;

class PID {
public:
    PID(ApplicationManager& appManager);
    ~PID();

    pid_t findByName(std::string processName);
    pid_t livePID();

    PID* init();

private:
    ApplicationManager& app_;
    LogHandler* log_;

    pid_t abletonLivePID = -1;
};

#endif
