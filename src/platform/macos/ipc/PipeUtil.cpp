#include "LogGlobal.h"
#include "IPCDefinitions.h"
#include "PipeUtil.h"

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

PipeUtil::PipeUtil()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeFlags_()
{
}

using Path = std::filesystem::path;

auto PipeUtil::createPipe() -> bool {
    logger->error("pipe name: " + this->getPath().string());
    Path directory = this->getPath().parent_path();

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
            return true;
        } else {
            logger->error("Failed to create named pipe: " + pipePath_.string() + " - " + strerror(errno));
            return false;
        }
    } else {
        logger->debug("Pipe created: " + pipePath_.string());
    }
    return true;
}

auto PipeUtil::openPipe() -> bool {
    pipeHandle_ = open(this->getPath().c_str(), this->getFlags()); // NOLINT
    if (pipeHandle_ == ipc::INVALID_PIPE_HANDLE) {
        logger->error("Failed to open pipe: " + this->getPath().string() + " - " + strerror(errno));
        return false;
    }

    this->setHandle(pipeHandle_);
    logger->debug("Pipe opened: " + this->getPath().string());
    logger->debug("Pipe opened handle: " + std::to_string(pipeHandle_));
    return true;
}

auto PipeUtil::closePipe() -> bool {
//    if (h != ipc::NULL_PIPE_HANDLE && h != ipc::INVALID_PIPE_HANDLE) {
    if (close(this->getHandle()) == -1) {
        std::string errorString = strerror(errno);
        this->setHandle(ipc::INVALID_PIPE_HANDLE);
        //throw std::runtime_error("Failed to close pipe: " + errorString);
        logger->error("failed to close pipe: " + errorString);
        return false;
    } else {
        this->setHandle(ipc::INVALID_PIPE_HANDLE);
        return true;
    }
}

auto PipeUtil::deletePipe() -> void {
    if (! (fs::exists(pipePath_)) ) {
        logger->warn("pipe does not exist or is not a regular file: " + pipePath_.string());
        return;
    }
    if (std::filesystem::remove(pipePath_)) {
        logger->debug("deleted pipe: " + pipePath_.string());
    } else {
        logger->warn("Failed to remove request pipe file: " + pipePath_.string());
    }
}

auto PipeUtil::ensurePipeOpen() -> bool {
    return true;
}

auto PipeUtil::drainPipe() -> void {
    std::vector<char> buffer(ipc::BUFFER_SIZE);
    ssize_t bytesRead = 0;
    do { // NOLINT - don't be stupid. Know and love the do-while
        bytesRead = read(this->getHandle(), const_cast<char*>(buffer.data()), buffer.size());
    } while (bytesRead > 0);
}

auto PipeUtil::writeToPipe(ipc::Request request) -> size_t {
    ssize_t result = ::write(
            this->getHandle()
            , request.formatted().c_str()
            , request.formatted().length()
    );
    if (result == -1) {
        if (errno == EAGAIN) {
            logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            logger->error("Failed to write to request pipe: " + this->getPath().string() + " - " + strerror(errno));
        }
        return false;
    } else if (result != request.formatted().length()) {
        logger->error("Incomplete write to request pipe. Wrote " + std::to_string(result) + " of " + std::to_string(request.formatted().length()) + " bytes");
        return false;
    }
    logger->debug("Request written successfully, bytes written: " + std::to_string(result));
    return static_cast<size_t>(result);
}

auto PipeUtil::readFromPipe(void* buffer, size_t count) const -> ssize_t {
    ssize_t result = ::read(this->getHandle(), buffer, count);
    if (result == -1) {
        throw std::runtime_error("Read error: " + std::string(strerror(errno)));
    }
    return result;
}

