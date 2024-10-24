#include <fcntl.h> // for flags

#include "PathManager.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"
#include "IPCRequestPipe.h"
#include "PipeUtil.h"

IPCRequestPipe::IPCRequestPipe(std::function<std::shared_ptr<PipeUtil>()> pipeUtil)
        : IPCPipe(std::move(pipeUtil))
        , pipeUtil_(std::move(pipeUtil))
        , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
        , pipePath_(PathManager().requestPipe())
        , pipeFlags_(O_WRONLY | O_NONBLOCK)
{
    auto p = pipeUtil_();
    p->setHandle(ipc::INVALID_PIPE_HANDLE);;
    p->setPath(PathManager().requestPipe());
    p->setFlags(O_WRONLY | O_NONBLOCK);


}

IPCRequestPipe::~IPCRequestPipe() = default;
