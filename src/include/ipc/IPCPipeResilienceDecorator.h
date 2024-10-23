#pragma once

#include <functional>
#include <memory>
#include <string>

#include "LogLevel.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"

class IPCPipeResilienceDecorator : public IPCPipe {
public:
    IPCPipeResilienceDecorator(std::function<std::shared_ptr<IPCPipe>()> factory);
    ~IPCPipeResilienceDecorator() override = default;

    IPCPipeResilienceDecorator(const IPCPipeResilienceDecorator &) = delete;
    IPCPipeResilienceDecorator(IPCPipeResilienceDecorator &&) = delete;
    auto operator=(const IPCPipeResilienceDecorator &) -> IPCPipeResilienceDecorator & = delete;
    auto operator=(IPCPipeResilienceDecorator &&) -> IPCPipeResilienceDecorator & = delete;

    auto readResponse(ipc::ResponseCallback callback) -> std::string override;

private:
    std::shared_ptr<IPCPipe> instance_;
    std::function<std::shared_ptr<IPCPipe>()> ipcFactory_;

    template<typename Func>
    auto withResilience(const std::string& operation, Func retry) -> decltype(retry());

    void logMessage(const std::string& message, LogLevel logLevel);
};
