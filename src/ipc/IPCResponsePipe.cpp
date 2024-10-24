#include <fcntl.h> // for flags

#include "PathManager.h"

#include "IPCDefinitions.h"
#include "PipeUtil.h"

#include "IPCPipe.h"
#include "IPCResponsePipe.h"

IPCResponsePipe::IPCResponsePipe(std::function<std::shared_ptr<PipeUtil>()> pipeUtil)
        : IPCPipe(std::move(pipeUtil()))
{
    IPCPipe::getPipeUtil()->setHandle(ipc::INVALID_PIPE_HANDLE);;
    IPCPipe::getPipeUtil()->setPath(PathManager().responsePipe());
    IPCPipe::getPipeUtil()->setFlags(O_RDONLY | O_NONBLOCK);
}

IPCResponsePipe::~IPCResponsePipe() = default;
