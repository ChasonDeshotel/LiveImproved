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

auto PipeUtil::getHandle() const -> ipc::Handle {
    return pipeHandle_;
}

auto PipeUtil::getPath() const -> const ipc::Path& {
    return pipePath_;
}

auto PipeUtil::getFlags() const -> int {
    return pipeFlags_;
}
