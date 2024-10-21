#include <string>

#include "LogGlobal.h"
#include "IPCException.h"
#include "IPCResilienceDecorator.h"
#include "Types.h"
#include "IIPC.h"

IPCResilienceDecorator::IPCResilienceDecorator(
    std::function<std::shared_ptr<IIPC>()> ipcFactory
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

auto IPCResilienceDecorator::logMessage(const std::string& message, LogLevel level) -> void {
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

auto IPCResilienceDecorator::checkAndReestablishConnection() -> bool {
    if (!isInitialized()) {
        auto log = logger;
        log->info("IPC connection lost or not initialized. Attempting to establish...");
        return init();
    }
    return true;
}

auto IPCResilienceDecorator::init() -> bool {
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

auto IPCResilienceDecorator::isInitialized() const -> bool {
    return instance_ && instance_->isInitialized();
}

auto IPCResilienceDecorator::writeRequest(const std::string& message) -> void {
    handleError("writeRequest", [this, &message]() { instance_->writeRequest(message); });
}

auto IPCResilienceDecorator::writeRequest(const std::string& message, ResponseCallback callback) -> void {
    handleError("writeRequest", [this, &message, &callback]() { instance_->writeRequest(message, callback); });
}

auto IPCResilienceDecorator::readResponse(ResponseCallback callback) -> std::string {
    return handleError("readResponse", [this, &callback]() { return instance_->readResponse(callback); });
}

auto IPCResilienceDecorator::stopIPC() -> void {
    handleError("stopIPC", [this]() { instance_->stopIPC(); });
}

 auto IPCResilienceDecorator::cleanUpPipes() -> void {
     handleError("cleanUpPipes", [this]() { instance_->cleanUpPipes(); });
 }

 auto IPCResilienceDecorator::createReadPipe() -> bool {
     return handleError("createReadPipe", [this]() { return instance_->createReadPipe(); });
 }

 auto IPCResilienceDecorator::createWritePipe() -> bool {
     return handleError("createWritePipe", [this]() { return instance_->createWritePipe(); });
 }
