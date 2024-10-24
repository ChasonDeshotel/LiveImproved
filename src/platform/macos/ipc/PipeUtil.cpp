#include "LogGlobal.h"
#include "IPCDefinitions.h"
#include "PipeUtil.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

PipeUtil::PipeUtil()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeFlags_()
{
}

using Path = std::filesystem::path;

auto PipeUtil::create() -> bool {
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

auto PipeUtil::setHandle(ipc::Handle handle) -> void {
    pipeHandle_ = handle;
}

auto PipeUtil::setPath(const ipc::Path& path) -> void {
    logger->error("setting path: " + path.string());
    pipePath_ = path;
}

auto PipeUtil::setFlags(int flags) -> void {
    logger->error("setting flags");
    pipeFlags_ = flags;
}

auto PipeUtil::getHandle() const -> ipc::Handle {
    return pipeHandle_;
}

auto PipeUtil::getPath() const -> const ipc::Path& {
    return pipePath_;
}

auto PipeUtil::getFlags() const -> int {
    return pipeFlags_;
}
