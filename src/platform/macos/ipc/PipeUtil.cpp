#include "IPCDefinitions.h"
#include "PipeUtil.h"

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

PipeUtil::PipeUtil()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeFlags_()
{
}

auto PipeUtil::create() -> bool {
    return true;
}

auto PipeUtil::setHandle(ipc::Handle handle) -> void {
    pipeHandle_ = handle;
}

auto PipeUtil::setPath(const ipc::Path& path) -> void {
    pipePath_ = path;
}

auto PipeUtil::setFlags(int flags) -> void {
    pipeFlags_ = flags;
}
