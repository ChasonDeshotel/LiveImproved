#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

#include "LogGlobal.h"
#include "DependencyContainer.h"

#include "IPCQueue.h"

IPCQueue::IPCQueue()
    : IIPC()
    , isProcessingRequest_(false)
    , requestPipe_(std::make_unique<IPCRequestPipe>())
    , responsePipe_(std::make_unique<IPCResponsePipe>())
{
    DependencyContainer::getInstance();
}

IPCQueue::~IPCQueue() {
    this->cleanUpPipes();
}

auto IPCQueue::init() -> bool {
    logger->debug("IPCQueue::init() called");
    stopIPC_ = false;

    std::thread createReadPipeThread(&IPCQueue::createReadPipeLoop, this);
    std::thread createWritePipeThread(&IPCQueue::createWritePipeLoop, this);

    {
        std::unique_lock<std::mutex> lock(createPipesMutex_);
        createPipesCv_.wait(lock, [this] { return (readPipeCreated_ && writePipeCreated_) || stopIPC_; });
    }

    createReadPipeThread.join();
    createWritePipeThread.join();

    if (!readPipeCreated_ || !writePipeCreated_) {
        logger->error("IPCQueue::init() failed to create pipes");
        return false;
    }

    std::thread readyReadThread(&IPCQueue::readyReadPipe, this);
    std::thread readyWriteThread(&IPCQueue::readyWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] { return (readPipeReady_ && writePipeReady_) || stopIPC_; });
    }

    readyReadThread.join();
    readyWriteThread.join();

    if (readPipeReady_ && writePipeReady_) {
        isInitialized_ = true;
        logger->info("IPCQueue::init() read/write enabled");
        return true;
    } else {
        logger->error("IPCQueue::init() failed");
        return false;
    }
}

auto IPCQueue::createReadPipeLoop() -> void {
    logger->debug("Creating read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->info("IPCQueue read pipe creation cancelled.");
            return;
        }
        if (createReadPipe()) {
            logger->info("Response pipe successfully created");
            readPipe
Created_ = true;
            createPipesCv_.notify_one();
            return;
        }
        logger->warn("Attempt to create response pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    logger->error("Max attempts reached for creating response pipe");
}

auto IPCQueue::createWritePipeLoop() -> void {
    logger->debug("Creating write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->info("IPCQueue write pipe creation cancelled.");
            return;
        }
        if (createWritePipe()) {
            logger->info("Request pipe successfully created");
            writePipeCreated_ = true;
            createPipesCv_.notify_one();
            return;
        }
        logger->warn("Attempt to create request pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    logger->error("Max attempts reached for creating request pipe");
}

auto IPCQueue::readyReadPipe() -> void {
    logger->debug("Setting up read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue read initialization cancelled.");
            return;
        }
        if (responsePipe_->openPipe()) {
            logger->info("Response pipe successfully opened for reading");
            readPipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
}

auto IPCQueue::readyWritePipe() -> void {
    logger->debug("Setting up write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue write initialization cancelled.");
            return;
        }
        if (requestPipe_->openPipe()) {
            logger->info("Request pipe successfully opened for writing");
            writePipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open request pipe for writing failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening request pipe");
}

auto IPCQueue::formatRequest(const std::string& message, uint64_t id) -> std::string {
    size_t messageLength = message.length();

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000); // NOLINT - magic numbers
    std::string paddedId = idStream.str();

    std::ostringstream markerStream;
    markerStream << "START_" << paddedId << std::setw(8) << std::setfill('0') << messageLength; // NOLINT - magic numbers
    std::string start_marker = markerStream.str();

    std::string formattedRequest = start_marker + message;
    logger->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "..."); // NOLINT - magic numbers
    formattedRequest += "\n"; // add newline as a delimiter

    return formattedRequest;
}

auto IPCQueue::writeRequest(const std::string& message, ResponseCallback callback) -> void {
    if (!isInitialized_) {
        logger->error("IPCQueue not initialized. Cannot write request.");
        return;
    }

    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(message, callback);
    lock.unlock();
    logger->debug("Request enqueued: " + message);

    if (!isProcessingRequest_) {
        processNextRequest();
    }
}

auto IPCQueue::processNextRequest() -> void {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (stopIPC_) {
        logger->info("Stopping IPCQueue request processing.");
        isProcessingRequest_ = false;
        return;
    }

    if (requestQueue_.empty()) {
        isProcessingRequest_ = false;
        return;
    }

    isProcessingRequest_ = true;

    auto nextRequest = requestQueue_.front();
    requestQueue_.pop();
    lock.unlock();

    logger->debug("Processing next request: " + nextRequest.first);
    std::thread([this, nextRequest]() {
        if (!stopIPC_) {
            this->writeRequestInternal(nextRequest.first, nextRequest.second);
        }

        std::this_thread::sleep_for(LIVE_TICK);

        if (!stopIPC_) {
            this->processNextRequest();
        }
    }).detach();
}

auto IPCQueue::writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool {
    responsePipe_->drainPipe(BUFFER_SIZE);

    uint64_t id = nextRequestId_++;

    std::string formattedRequest;
    try {
        formattedRequest = formatRequest(message, id);
        logger->debug("Request formatted successfully");
    } catch (const std::exception& e) {
        logger->error("Exception in formatRequest: " + std::string(e.what()));
        return false;
    }

    logger->debug("Writing request: " + formattedRequest);

    if (!requestPipe_->write(formattedRequest)) {
        logger->error("Failed to write to request pipe");
        return false;
    }

    logger->debug("Request written successfully");

    std::thread readerThread([this, callback]() {
        std::this_thread::sleep_for(LIVE_TICK * 2);
        this->readResponse(callback);
    });
    readerThread.detach();

    return true;
}

auto IPCQueue::readResponse(ResponseCallback callback) -> std::string {
    logger->debug("IPCQueue::readResponse() called");

    std::string header = responsePipe_->read(HEADER_SIZE);
    if (header.empty()) {
        logger->error("Failed to read response header");
        return "";
    }

    logger->debug("Full header received: " + header);

    size_t messageSize = 0;
    try {
        std::string messageSizeStr(header.substr(14, 8));  // NOLINT Skip 'START_' and the 8 characters of request ID
        messageSize = std::stoull(messageSizeStr);
    } catch (const std::exception& e) {
        logger->error("Invalid header. Could not parse message size: " + std::string(e.what()));
        return "";
    }

    logger->debug("Message size to read: " + std::to_string(messageSize));

    std::string message = responsePipe_->read(messageSize + END_MARKER.size());
    if (message.empty()) {
        logger->error("Failed to read response message");
        return "";
    }

    if (message.compare(message.size() - END_MARKER.size(), END_MARKER.size(), END_MARKER) == 0) {
        message = message.substr(0, message.size() - END_MARKER.size());
    } else {
        logger->warn("End of message marker not found");
    }

    logger->debug("Message read from response pipe");
    if (message.length() > MESSAGE_TRUNCATE_CHARS) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, MESSAGE_TRUNCATE_CHARS));
    } else {
        logger->debug("Message: " + message);
    }

    if (callback && !stopIPC_) {
        callback(message);
    }

    return message;
}

auto IPCQueue::cleanUpPipes() -> void {
    requestPipe_->cleanUp();
    responsePipe_->cleanUp();
}

auto IPCQueue::createReadPipe() -> bool {
    return responsePipe_->create();
}

auto IPCQueue::createWritePipe() -> bool {
    return requestPipe_->create();
}

