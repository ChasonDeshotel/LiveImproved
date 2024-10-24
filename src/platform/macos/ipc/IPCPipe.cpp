#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"

namespace fs = std::filesystem;

IPCPipe::IPCPipe()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeFlags_()
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

auto IPCPipe::openPipe() -> bool {
    pipeHandle_ = open(pipePath_.c_str(), getPipeFlags()); // NOLINT
    if (pipeHandle_ == ipc::INVALID_PIPE_HANDLE) {
        logger->error("Failed to open pipe: " + pipePath_.string() + " - " + strerror(errno));
        return false;
    }

    setHandle(pipeHandle_);
    logger->debug("Pipe opened: " + pipePath_.string());
    logger->debug("Pipe opened handle: " + std::to_string(pipeHandle_));
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

auto IPCPipe::drainPipe(size_t bufferSize) -> void {
    std::vector<char> buffer(bufferSize);
    ssize_t bytesRead = 0;
    do { // NOLINT - don't be stupid. Know and love the do-while
        bytesRead = read(pipeHandle_, const_cast<char*>(buffer.data()), buffer.size());
    } while (bytesRead > 0);
}

// loops openPipe
auto IPCPipe::openPipeLoop() -> bool {
    logger->debug("Setting up pipe. Path: " + pipePath_.string());
    for (int attempt = 0; attempt < ipc::MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue initialization cancelled.");
            return false;
        }
        if (openPipe()) {
            logger->info("Pipe successfully opened");

            // TODO
            // if response pipe
            // setHandleNull()

            return true;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(ipc::PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
    return false;
}

auto IPCPipe::readResponse(ipc::ResponseCallback callback) -> ipc::Response {
    logger->error("not implemented. Must be called from IPCResponsePipe.");
    return {ipc::ResponseType::Error, std::nullopt};
}

auto IPCPipe::writeRequest(ipc::Request request) -> bool {
    logger->error("not implemented. Must be called from IPCResponsePipe.");
    return false;
}
