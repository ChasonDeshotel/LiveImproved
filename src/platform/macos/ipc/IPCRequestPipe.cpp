#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCPipe.h"
#include "IPCRequestPipe.h"

IPCRequestPipe::IPCRequestPipe()
    : IPCPipe()
    , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_(PathManager().requestPipe())
{
    // set on base class
    setPipePath(pipePath_);
}

IPCRequestPipe::~IPCRequestPipe() = default;

auto IPCRequestPipe::openPipe() -> bool {
    int flags = O_WRONLY;
    flags |= O_NONBLOCK;

    pipeHandle_ = open(pipePath_.c_str(), flags); // NOLINT
    if (pipeHandle_ == ipc::INVALID_PIPE_HANDLE) {
        logger->error("Failed to open pipe for writing: " + pipePath_.string() + " - " + strerror(errno));
        return false;
    }

    logger->debug("Pipe opened for writing: " + pipePath_.string());
    return true;
}

