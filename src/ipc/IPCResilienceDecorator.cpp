#include <string>

#include "LogGlobal.h"
#include "IPCException.h"
#include "IPCResilienceDecorator.h"
#include "Types.h"
#include "IIPCCore.h"

IPCResilienceDecorator::IPCResilienceDecorator(
    std::function<std::shared_ptr<IIPCCore>()> ipcFactory
) : ipcFactory_(std::move(ipcFactory)),
    instance_(nullptr)
{
    logger->debug("IPCResilienceDecorator constructed");
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
        logger->error("Unexpected error in IPC " + operation + ": " + e.what());
        throw; // Re-throw unexpected exceptions
    }
}

void IPCResilienceDecorator::logMessage(const std::string& message, LogLevel level) {
    switch (level) {
        case LogLevel::LOG_DEBUG:
            logger->debug(message);
            break;
        case LogLevel::LOG_INFO:
            logger->info(message);
            break;
        case LogLevel::LOG_WARN:
            logger->warn(message);
            break;
        case LogLevel::LOG_ERROR:
            logger->error(message);
            break;
        case LogLevel::LOG_TRACE:
        case LogLevel::LOG_FATAL:
            logger->error(message);
            break;
    }
}

bool IPCResilienceDecorator::checkAndReestablishConnection() {
    if (!isInitialized()) {
        auto log = logger;
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
                logger->error("Failed to create IPC instance");
                return false;
            }
        }

        if (!instance_->init()) {
            logger->error("Failed to initialize IPC instance");
            instance_ = nullptr;  // Reset the instance so we can try again later
            return false;
        }

        logger->info("IPC connection established successfully");
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
