#ifndef PROCESS_H
#define PROCESS_H

#include <string>

#ifdef _WIN32
	#include <windows.h>
	typedef DWORD pid_t;
#else
	#include <sys/types.h>
	#include <unistd.h>
#endif

class LogHandler;

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

#endif
