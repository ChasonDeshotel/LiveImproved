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

    auto writeToPipe(ipc::Request) -> bool override;

    auto readResponse(ipc::ResponseCallback callback) -> ipc::Response override;

    auto logMessage(const std::string& message) -> void;

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
