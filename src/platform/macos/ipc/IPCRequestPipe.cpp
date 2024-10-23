#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCDefinitions.h"
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

auto IPCRequestPipe::writeToPipe(ipc::Request request) -> bool {
	if (this->getHandle() == ipc::INVALID_PIPE_HANDLE) {
		if (!this->openPipe()) {
		    logger->error("Request pipe not opened for writing: " + this->string());
			return false;
		}
	}

    this->drainPipe(ipc::BUFFER_SIZE);

    logger->debug("Writing request: " + request.formatted());

    ssize_t bytesWritten = write(this->getHandle(), request.formatted().c_str(), request.formatted().length());
    if (bytesWritten == -1) {
        if (errno == EAGAIN) {
            logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            logger->error("Failed to write to request pipe: " + this->string() + " - " + strerror(errno));
        }
        return false;
    } else if (bytesWritten != request.formatted().length()) {
        logger->error("Incomplete write to request pipe. Wrote " + std::to_string(bytesWritten) + " of " + std::to_string(request.formatted().length()) + " bytes");
        return false;
    }
    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));

    return true;
}

auto IPCRequestPipe::readResponse(ipc::ResponseCallback dummy) -> ipc::Response {
    logger->error("unimplemented in response pipe");
    return {ipc::ResponseType::Error, std::nullopt};
}
