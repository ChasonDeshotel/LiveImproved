#include <cerrno>
#include <thread>
#include <fcntl.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "PipeUtil.h"

#include "IPCPipe.h"
#include "IPCDefinitions.h"
#include "IPCResponsePipe.h"

using ipc::Response;

IPCResponsePipe::IPCResponsePipe(std::function<std::shared_ptr<PipeUtil>()> pipeUtil)
        : IPCPipe(std::move(pipeUtil))
        , pipeUtil_(std::move(pipeUtil))
        , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
        , pipePath_(PathManager().responsePipe())
        , pipeFlags_(O_RDONLY | O_NONBLOCK)
{
    auto p = pipeUtil_();
    p->setHandle(ipc::INVALID_PIPE_HANDLE);;
    p->setPath(PathManager().responsePipe());
    p->setFlags(O_RDONLY | O_NONBLOCK);
}

IPCResponsePipe::~IPCResponsePipe() = default;


//auto IPCResponsePipe::writeRequest(ipc::Request) -> bool {
//    logger->error("unimplemented in response pipe");
//    return false;
//}

