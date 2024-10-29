#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <string>

#include "IPCDefinitions.h"
class PipeUtil;

class IPCPipe {
public:
    IPCPipe(std::shared_ptr<PipeUtil> pipeUtil);
    virtual ~IPCPipe() = default;

    IPCPipe(const IPCPipe &) = delete;
    IPCPipe(IPCPipe &&) = delete;
    auto operator=(const IPCPipe &) -> IPCPipe & = delete;
    auto operator=(IPCPipe &&) -> IPCPipe & = delete;

    auto getPipeUtil() -> std::shared_ptr<PipeUtil>;

    auto cleanUp()  -> void;

    virtual auto openPipe() -> bool;

    auto drainPipe() -> void;

    auto stop() -> void {
        stopIPC_ = true;
    }

    auto create() -> bool;
    auto openPipeLoop() -> bool;

    auto readHeader() -> std::optional<ipc::Header>;
    auto readMessage(size_t messageSize) -> std::optional<std::string>;

    virtual auto readResponse(ipc::ResponseCallback callback) -> ipc::Response;
    virtual auto writeRequest(ipc::Request) -> bool;
    auto logMessage(const std::string& message) -> void;

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    std::shared_ptr<PipeUtil> p_;

    ipc::Path     pipePath_;
    ipc::Handle   pipeHandle_;
    ipc::PipeMode pipeMode_;
};
