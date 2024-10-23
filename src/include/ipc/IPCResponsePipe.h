#pragma once

#include "IPCDefinitions.h"
#include "IPCPipe.h"

class IPCResponsePipe : public IPCPipe {
public:
    IPCResponsePipe();
    ~IPCResponsePipe() override;

    IPCResponsePipe(const IPCResponsePipe &) = delete;
    IPCResponsePipe(IPCResponsePipe &&) = delete;
    IPCResponsePipe &operator=(const IPCResponsePipe &) = delete;
    IPCResponsePipe &operator=(IPCResponsePipe &&) = delete;

    auto writeToPipe(const std::string& message, ipc::ResponseCallback callback) -> bool override;

    auto readResponse(ipc::ResponseCallback callback) -> std::string override;

    auto logMessage(const std::string& message) -> void;

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
