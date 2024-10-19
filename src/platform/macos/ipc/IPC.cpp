#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"

#include "IPC.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

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
    logger->debug("pipe name: " + pipeFilePath.string());
    std::filesystem::path directory = pipeFilePath.parent_path();

    // Check if the directory exists, and create it if it doesn't
    struct stat st{};
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), DEFAULT_DIRECTORY_PERMISSIONS) == -1) {
            logger->error("Failed to create directory: " + directory.string() + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipeFilePath.c_str(), DEFAULT_PIPE_PERMISSIONS) == -1) {
        if (errno == EEXIST) {
            logger->warn("Pipe already exists: " + pipeFilePath.string());
        } else {
            logger->error("Failed to create named pipe: " + pipeFilePath.string() + " - " + strerror(errno));
            return false;
        }
    } else {
        logger->debug("Pipe created: " + pipeFilePath.string());
    }

    return true;
}

auto IPC::cleanUpPipe(const Path& path, PipeHandle& handle) -> void {
    if (handle != NULL_PIPE_HANDLE && handle != INVALID_PIPE_HANDLE) {
        close(handle);
        logger->debug("closed pipe");
    }
    handle = NULL_PIPE_HANDLE;

    if (! (fs::exists(path) && fs::is_regular_file(path)) ) {
        logger->warn("pipe does not exist or is not a regular file: " + path.string());
        return;
    }
    if (std::filesystem::remove(_requestPipePath_)) {
        logger->debug("deleted pipe: " + path.string());
    } else {
        logger->warn("Failed to remove request pipe file: " + path.string());
    }

    handle = INVALID_PIPE_HANDLE;
}
