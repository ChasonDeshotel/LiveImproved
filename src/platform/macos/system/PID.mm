#include <ApplicationServices/ApplicationServices.h>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>
#include <iostream>
#include <fstream>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <libproc.h>
#include <dispatch/dispatch.h>
#include <unistd.h>
#include <vector>

#include "LogGlobal.h"

#include "PID.h"

PID::PID()
    : abletonLivePID_(-1)
{}

PID::~PID() {}

PID& PID::getInstance() {
    static PID instance;
    return instance;
}

bool PID::isLIMRunning() {
    return findLiveImproveds().size() > 0;
}

std::vector<pid_t> PID::findLiveImproveds() {
    std::vector<pid_t> out;

    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t len;

    if (sysctl(mib, 4, NULL, &len, NULL, 0) < 0) {
        logger->error("sysctl failed to get process list size");
        return {};
    }

    struct kinfo_proc *procs = (struct kinfo_proc *)malloc(len);

    if (sysctl(mib, 4, procs, &len, NULL, 0) < 0) {
        logger->error("sysctl failed to get process list");
        free(procs);
        return {};
    }

    int procCount = static_cast<int>(len / sizeof(struct kinfo_proc));
    
    for (int i = 0; i < procCount; i++) {
        struct kinfo_proc *proc = &procs[i];
        if (strcmp(proc->kp_proc.p_comm, "LiveImproved") == 0) {
            if (proc->kp_proc.p_stat == SZOMB) continue; // skip zombies
            pid_t pid = proc->kp_proc.p_pid;
            out.emplace_back(pid);
            logger->info("LiveImproved found with PID: {}", std::to_string(pid));
        }
    }

    free(procs);
    return out;
}

bool PID::isLiveRunning() {
    return findLivePIDNoCache() != -1;
}

pid_t PID::findLivePID() {
    if (abletonLivePID_ != -1) {
      return abletonLivePID_;
    }
    auto pid = findLivePIDNoCache();
    abletonLivePID_ = pid;
    return pid;
}

pid_t PID::findLivePIDNoCache() {
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t len;

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
        if (strcmp(proc->kp_proc.p_comm, "Live") == 0) {
            if (proc->kp_proc.p_stat == SZOMB) continue; // skip zombies
            pid_t pid = proc->kp_proc.p_pid;
            free(procs);
            logger->info("Ableton Live found with PID: {}", std::to_string(pid));
            return pid;
        }
    }

    free(procs);
    logger->error("Ableton Live not found");
    return -1;
}

pid_t PID::livePID() {
//    logger->info("livePID() called");
    if (abletonLivePID_ != -1) {
//      logger->info("PID::livePID() - returning cached result");
      return abletonLivePID_;
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

void PID::clearLivePID() {
    abletonLivePID_ = -1;
}