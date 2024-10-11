#include <ApplicationServices/ApplicationServices.h>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>
#include <iostream>
#include <fstream>
#include <sys/sysctl.h>
#include <libproc.h>
#include <dispatch/dispatch.h>
#include <unistd.h>

#include "LogGlobal.h"

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
//    logger->info("PID::findWithSysctl() called");

    if (abletonLivePID != -1) {
//      logger->info("PID::findByName() - returning cached result");
      return abletonLivePID;
    }

    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t len;

    // Get the size of the process list
    if (sysctl(mib, 4, NULL, &len, NULL, 0) < 0) {
        logger->error("sysctl failed to get process list size");
        return -1;
    }

    struct kinfo_proc *procs = (struct kinfo_proc *)malloc(len);

    if (sysctl(mib, 4, procs, &len, NULL, 0) < 0) {
        logger->error("sysctl failed to get process list");
        free(procs);
        return -1;
    }

    int procCount = static_cast<int>(len / sizeof(struct kinfo_proc));
    
    for (int i = 0; i < procCount; i++) {
        struct kinfo_proc *proc = &procs[i];
//        logger->info(proc->kp_proc.p_comm);
        if (strcmp(proc->kp_proc.p_comm, "Live") == 0) {
            pid_t pid = proc->kp_proc.p_pid;
            free(procs);
            logger->info("Ableton Live found with PID: " + std::to_string(pid));
            abletonLivePID = pid;
            return pid;
        }
    }

    free(procs);
    logger->error("Ableton Live not found");
    return -1;
}

pid_t PID::livePID() {
//    logger->info("livePID() called");
    if (abletonLivePID != -1) {
//      logger->info("PID::livePID() - returning cached result");
      return abletonLivePID;
    }

    return findLivePID();
}

pid_t PID::appPID() {
    return getpid();
}

PID* PID::livePIDBlocking() {
    logger->debug("PID::Init() called");

    while (livePID() == -1) {
        logger->info("Live not found, retrying...");
        livePID();
        sleep(1);
    }
  
    return this;
}
