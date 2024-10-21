#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCDefinitions.h"
#include "IPCResponsePipe.h"

IPCResponsePipe::IPCResponsePipe()
    : IPCPipe()
    , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_(PathManager().responsePipe())
{}

IPCResponsePipe::~IPCResponsePipe() = default;

auto IPCResponsePipe::openPipe() -> bool {
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

//auto IPCRequestPipeBase::resetResponsePipe() -> void {
//    logger->debug("Resetting response pipe");
//    close(_responsePipeHandle_);
//    _responsePipeHandle_ = INVALID_PIPE_HANDLE;
//    if (!IPCRequestPipe::openResponsePipe(_responsePipePath_, _responsePipeHandle_)) {
//        logger->error("Failed to reopen response pipe");
//    } else {
//        logger->info("Response pipe reopened successfully");
//    }
//}
//auto IPCRequestPipeBase::removePipeIfExists(const std::string& pipeName) -> void {
//    // TODO: use filesystem remove
//    if (access(pipeName.c_str(), F_OK) != -1) {
//        // File exists, so remove it
//        if (unlink(pipeName.c_str()) == 0) {
//            logger->debug("Removed existing pipe: " + pipeName);
//        } else {
//            logger->error("Failed to remove existing pipe: " + pipeName + " - " + strerror(errno));
//        }
//    }
//}
