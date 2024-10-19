#include <filesystem>

#include "LogGlobal.h"

#include "IPC.h"

using Path = std::filesystem::path;
using fs   = std::filesystem;

IPC::IPC() {
}

IPC::~IPC() {

}

auto IPC::createReadPipe() -> bool {
    return createPipe(_responsePipePath_);
}

auto IPC::createWritePipe() -> bool {
    return createPipe(_requestPipePath_);
}

auto IPC::createPipe(const Path& pipeFilePath) -> bool {
    // Extract the directory path from the pipeFilePath
    logger->debug("pipe name: " + pipeFilePath);
    std::string directory = pipeFilePath.substr(0, pipeFilePath.string().find_last_of('/'));

    // Check if the directory exists, and create it if it doesn't
    struct stat st{};
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), DEFAULT_DIRECTORY_PERMISSIONS) == -1) {
            logger->error("Failed to create directory: " + directory + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipeFilePath.c_str(), DEFAULT_PIPE_PERMISSIONS) == -1) {
        if (errno == EEXIST) {
            logger->warn("Pipe already exists: " + pipeFilePath);
        } else {
            logger->error("Failed to create named pipe: " + pipeFilePath + " - " + strerror(errno));
            return false;
        }
    } else {
        logger->debug("Pipe created: " + pipeFilePath);
    }

    return true;
}


auto IPCBase::closePipe(PipeHandle handle) -> void {
    if (handle != NULL_PIPE_HANDLE && handle != INVALID_PIPE_HANDLE) {
        close(_requestPipeHandle_);
        logger->debug("Closed request pipe");
    }
    _requestPipeHandle_ = INVALID_PIPE_HANDLE;
}

auto IPCBase::removePipe(Path path) -> void {
    if (handle != NULL_PIPE_HANDLE && handle != INVALID_PIPE_HANDLE) {
        close(_requestPipeHandle_);
        logger->debug("Closed request pipe");
    }
    _requestPipeHandle_ = INVALID_PIPE_HANDLE;
}

     // Remove the pipe files from the filesystem
     if (std::filesystem::remove(_requestPipePath_)) {
         logger->debug("Removed request pipe file: " + _requestPipePath_);
     } else {
         logger->warn("Failed to remove request pipe file: " + _requestPipePath_);
     }

     if (std::filesystem::remove(_responsePipePath_)) {
         logger->debug("Removed response pipe file: " + _responsePipePath_);
     } else {
         logger->warn("Failed to remove response pipe file: " + _responsePipePath_);
     }
 }
