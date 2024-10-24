#include <fcntl.h> // for flags

#include "PathManager.h"

#include "IPCDefinitions.h"
#include "PipeUtil.h"

#include "IPCPipe.h"
#include "IPCRequestPipe.h"

IPCRequestPipe::IPCRequestPipe(std::function<std::shared_ptr<PipeUtil>()> pipeUtil)
        : IPCPipe(std::move(pipeUtil()))
{
    IPCPipe::getPipeUtil()->setHandle(ipc::INVALID_PIPE_HANDLE);;
    IPCPipe::getPipeUtil()->setPath(PathManager().requestPipe());
    IPCPipe::getPipeUtil()->setMode(ipc::PipeMode::Write);
}

IPCRequestPipe::~IPCRequestPipe() = default;
