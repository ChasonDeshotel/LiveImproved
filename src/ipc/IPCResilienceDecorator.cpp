#include <string>

#include "ILogHandler.h"
#include "IPCException.h"
#include "IPCResilienceDecorator.h"
#include "Types.h"
#include "IIPCCore.h"

IPCResilienceDecorator::IPCResilienceDecorator(
    std::function<std::shared_ptr<IIPCCore>()> ipcFactory,
    std::function<std::shared_ptr<ILogHandler>()> logHandler
) : ipcFactory_(std::move(ipcFactory)),
    logHandler_(std::move(logHandler)),
    instance_(nullptr)
{
    auto log = logHandler_();
    log->debug("IPCResilienceDecorator constructed");
    init();
}

template<typename Func>
auto IPCResilienceDecorator::handleError(const std::string& operation, Func retry) -> decltype(retry()) {
    try {
        return retry();
    } catch (const IPCException& e) {
        logMessage(e.what(), e.getLogLevel());

        if (!checkAndReestablishConnection()) {
            throw std::runtime_error("Failed to reestablish IPC connection after: " + std::string(e.what()));
        }

        // Retry the operation once more
        return retry();
    } catch (const std::exception& e) {
        logHandler_()->error("Unexpected error in IPC " + operation + ": " + e.what());
        throw; // Re-throw unexpected exceptions
    }
}

void IPCResilienceDecorator::logMessage(const std::string& message, LogLevel level) {
    switch (level) {
        case LogLevel::LOG_DEBUG:
            logHandler_()->debug(message);
            break;
        case LogLevel::LOG_INFO:
            logHandler_()->info(message);
            break;
        case LogLevel::LOG_WARN:
            logHandler_()->warn(message);
            break;
        case LogLevel::LOG_ERROR:
            logHandler_()->error(message);
            break;
        case LogLevel::LOG_TRACE:
        case LogLevel::LOG_FATAL:
            logHandler_()->error(message);
            break;
    }
}

bool IPCResilienceDecorator::checkAndReestablishConnection() {
    if (!isInitialized()) {
        auto log = logHandler_();
        log->info("IPC connection lost or not initialized. Attempting to establish...");
        return init();
    }
    return true;
}

bool IPCResilienceDecorator::init() {
    return handleError("init", [this]() {
        if (!instance_) {
            instance_ = ipcFactory_();
            if (!instance_) {
                logHandler_()->error("Failed to create IPC instance");
                return false;
            }
        }

        if (!instance_->init()) {
            logHandler_()->error("Failed to initialize IPC instance");
            instance_ = nullptr;  // Reset the instance so we can try again later
            return false;
        }

        logHandler_()->info("IPC connection established successfully");
        return true;
    });
}

bool IPCResilienceDecorator::isInitialized() const {
    return instance_ && instance_->isInitialized();
}

void IPCResilienceDecorator::writeRequest(const std::string& message) {
    handleError("writeRequest", [this, &message]() { instance_->writeRequest(message); });
}

void IPCResilienceDecorator::writeRequest(const std::string& message, ResponseCallback callback) {
    handleError("writeRequest", [this, &message, &callback]() { instance_->writeRequest(message, callback); });
}

std::string IPCResilienceDecorator::readResponse(ResponseCallback callback) {
    return handleError("readResponse", [this, &callback]() { return instance_->readResponse(callback); });
}

void IPCResilienceDecorator::drainPipe(int fd) {
    handleError("drainPipe", [this, fd]() { instance_->drainPipe(fd); });
}

void IPCResilienceDecorator::closeAndDeletePipes() {
    handleError("closeAndDeletePipes", [this]() { instance_->closeAndDeletePipes(); });
}

void IPCResilienceDecorator::stopIPC() {
    handleError("stopIPC", [this]() { instance_->stopIPC(); });
}
