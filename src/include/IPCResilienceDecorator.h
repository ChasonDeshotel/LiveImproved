 #pragma once

#include <string>

#include "IIPCCore.h"
#include "ILogHandler.h"
#include "Types.h"
#include <functional>
#include <memory>

class IPCResilienceDecorator : public IIPCCore {
public:
    IPCResilienceDecorator(
        std::function<std::shared_ptr<IIPCCore>()> baseIPC
        , std::function<std::shared_ptr<ILogHandler>()> logHandler
    );
    IPCResilienceDecorator(std::function<std::shared_ptr<IIPCCore>()> factory);
    ~IPCResilienceDecorator() override = default;

    bool init() override;
    bool isInitialized() const override;

    void writeRequest(const std::string& message) override;
    void writeRequest(const std::string& message, ResponseCallback callback) override;

    std::string readResponse(ResponseCallback callback) override;
    void drainPipe(int fd) override;
    void closeAndDeletePipes() override;

    void stopIPC() override;

    bool checkAndReestablishConnection();

private:
    std::shared_ptr<IIPCCore> instance_;
    std::function<std::shared_ptr<IIPCCore>()> ipcFactory_;
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;

    template<typename Func>
    auto handleError(const std::string& operation, Func retry) -> decltype(retry());

    void logMessage(const std::string& message, LogLevel logLevel);
};
