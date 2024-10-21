#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"

namespace fs = std::filesystem;

IPCPipe::IPCPipe()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
{}

auto IPCPipe::create() -> bool {
    logger->error("pipe name: " + getPipePath().string());
    Path directory = pipePath_.parent_path();

    // Check if the directory exists, and create it if it doesn't
    struct stat st{};
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), ipc::DEFAULT_DIRECTORY_PERMISSIONS) == -1) {
            logger->error("Failed to create directory: " + directory.string() + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipePath_.c_str(), ipc::DEFAULT_PIPE_PERMISSIONS) == -1) {
        if (errno == EEXIST) {
            logger->warn("Pipe already exists: " + pipePath_.string());
        } else {
            logger->error("Failed to create named pipe: " + pipePath_.string() + " - " + strerror(errno));
            return false;
        }
    } else {
        logger->debug("Pipe created: " + pipePath_.string());
    }

    return true;
}

auto IPCPipe::cleanUp() -> void {
    if (pipeHandle_ != ipc::NULL_PIPE_HANDLE && pipeHandle_ != ipc::INVALID_PIPE_HANDLE) {
        close(pipeHandle_);
        logger->debug("closed pipe");
    }
    setHandleNull();

    if (! (fs::exists(pipePath_)) ) {
        logger->warn("pipe does not exist or is not a regular file: " + pipePath_.string());
        return;
    }
    if (std::filesystem::remove(pipePath_)) {
        logger->debug("deleted pipe: " + pipePath_.string());
    } else {
        logger->warn("Failed to remove request pipe file: " + pipePath_.string());
    }

    setHandleNull();
}

auto IPCPipe::drainPipe(int fd, size_t bufferSize) -> void {
    std::vector<char> buffer(bufferSize);
    ssize_t bytesRead = 0;
    do { // NOLINT - don't be stupid. Know and love the do-while
        bytesRead = read(fd, const_cast<char*>(buffer.data()), buffer.size());
    } while (bytesRead > 0);
}

auto IPCPipe::getHandle() -> ipc::Handle {
    return pipeHandle_;
}

auto IPCPipe::setHandleNull() -> void {
    pipeHandle_ = ipc::NULL_PIPE_HANDLE;
}

auto IPCPipe::string() -> std::string {
    return pipePath_.string();
}

//auto IPCPipeBase::resetResponsePipe() -> void {
//    logger->debug("Resetting response pipe");
//    close(_responsePipeHandle_);
//    _responsePipeHandle_ = INVALID_PIPE_HANDLE;
//    if (!IPCPipe::openResponsePipe(_responsePipePath_, _responsePipeHandle_)) {
//        logger->error("Failed to reopen response pipe");
//    } else {
//        logger->info("Response pipe reopened successfully");
//    }
//}
//auto IPCPipeBase::removePipeIfExists(const std::string& pipeName) -> void {
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
