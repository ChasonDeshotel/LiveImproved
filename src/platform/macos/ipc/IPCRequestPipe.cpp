#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PathManager.h"

#include "IPCPipe.h"
#include "IPCRequestPipe.h"

IPCRequestPipe::IPCRequestPipe()
    : IPCPipe()
    , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_(PathManager().requestPipe())
    , pipeFlags_(O_WRONLY | O_NONBLOCK)
{
    // set on base class
    setPipePath(pipePath_);
    setPipeFlags(pipeFlags_);
}

IPCRequestPipe::~IPCRequestPipe() = default;

