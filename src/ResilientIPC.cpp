#include <string>

#include "ILogHandler.h"
#include "IPCException.h"
#include "ResilientIPC.h"
#include "include/Types.h"
#include "interface/IIPC.h"

ResilientIPC::ResilientIPC(
    std::function<std::shared_ptr<IIPC>()> ipcFactory
    , std::function<std::shared_ptr<ILogHandler>()> logHandler
    )
    : ipcFactory_(ipcFactory)
    , logHandler_(std::move(logHandler))
{
    instance_ = ipcFactory_();
}

template<typename Func>
auto ResilientIPC::handleError(const std::string& operation, Func retry) -> decltype(retry()) {
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

void ResilientIPC::logMessage(const std::string& message, LogLevel level) {
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
            // Handle these cases as appropriate for your application
            logHandler_()->error(message);  // Default to error for now
            break;
    }
}

bool ResilientIPC::checkAndReestablishConnection() {
    if (!instance_->isInitialized()) {
        logHandler_()->info("IPC connection lost. Attempting to reestablish...");
        instance_ = ipcFactory_();
        return instance_->init();
    }
    return true;
}

bool ResilientIPC::init() {
    return handleError("init", [this]() { return instance_->init(); });
}

bool ResilientIPC::isInitialized() const {
    return instance_->isInitialized();
}

void ResilientIPC::writeRequest(const std::string& message) {
    handleError("writeRequest", [this, &message]() { instance_->writeRequest(message); });
}

void ResilientIPC::writeRequest(const std::string& message, ResponseCallback callback) {
    handleError("writeRequest", [this, &message, &callback]() { instance_->writeRequest(message, callback); });
}

std::string ResilientIPC::readResponse(ResponseCallback callback) {
    return handleError("readResponse", [this, &callback]() { return instance_->readResponse(callback); });
}

void ResilientIPC::drainPipe(int fd) {
    handleError("drainPipe", [this, fd]() { instance_->drainPipe(fd); });
}

void ResilientIPC::closeAndDeletePipes() {
    handleError("closeAndDeletePipes", [this]() { instance_->closeAndDeletePipes(); });
}

void ResilientIPC::stopIPC() {
    handleError("stopIPC", [this]() { instance_->stopIPC(); });
}
