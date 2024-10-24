#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PathManager.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"
#include "IPCRequestPipe.h"
#include "PipeUtil.h"

IPCRequestPipe::IPCRequestPipe(
        std::function<std::shared_ptr<PipeUtil>(
            ipc::Handle
            , const ipc::Path&
            , int
        )> ipcUtil)
        : IPCPipe([ipcUtil, this] { return ipcUtil(pipeHandle_, pipePath_, pipeFlags_); })
        , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
        , pipePath_(PathManager().requestPipe())
        , pipeFlags_(O_WRONLY | O_NONBLOCK)
{}

IPCRequestPipe::~IPCRequestPipe() = default;
