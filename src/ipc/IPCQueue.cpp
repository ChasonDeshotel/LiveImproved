#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sstream>
#include <thread>

#include "LogGlobal.h"

#include "IPCDefinitions.h"
#include "IPCQueue.h"
#include "IPCRequestPipe.h"
#include "IPCResponsePipe.h"

IPCQueue::IPCQueue(
         std::function<std::shared_ptr<IPCRequestPipe>()> requestPipe
         , std::function<std::shared_ptr<IPCResponsePipe>()> responsePipe
        )
    : IIPC()
    , isProcessingRequest_(false)
    , requestPipe_(std::move(requestPipe)())
    , responsePipe_(std::move(responsePipe)())
    , currentState_(ipc::QueueState::Initializing)
{
    init();
}

IPCQueue::~IPCQueue() {
    this->cleanUpPipes();
}

auto IPCQueue::init() -> bool {
    logger->debug("IPCQueue::init() called");

    // create the pipes
    if(!requestPipe_->create() || !responsePipe_->create()) {
        logger->error("IPCQueue::init() failed to create pipes");
    }

    std::thread readyRequestThread(&IPCQueue::readyRequestWrapper, this);
    std::thread readyResponseThread(&IPCQueue::readyResponseWrapper, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] { return (requestPipeReady_ && responsePipeReady_) || stopIPC_; });
    }

    readyRequestThread.join();
    readyResponseThread.join();

    if (requestPipeReady_ && responsePipeReady_) {
        logger->info("IPCQueue::init() read/write enabled");
        setState(ipc::QueueState::Running);
        return true;
    } else {
        logger->error("IPCQueue::init() failed");
        setState(ipc::QueueState::Halted);
        return false;
    }
}

auto IPCQueue::readyRequestWrapper() -> void {
    if (requestPipe_->openPipeLoop()) {
        std::lock_guard<std::mutex> lock(initMutex_);
        requestPipeReady_ = true;
        initCv_.notify_one();
    }
    return;
}

auto IPCQueue::readyResponseWrapper() -> void {
    if (responsePipe_->openPipeLoop()) {
        std::lock_guard<std::mutex> lock(initMutex_);
        responsePipeReady_ = true;
        initCv_.notify_one();
        return;
    }
    return;
}

auto IPCQueue::formatRequest(const std::string& message, uint64_t id) -> std::string {
    size_t messageLength = message.length();

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000); // NOLINT - magic numbers
    std::string paddedId = idStream.str();

    std::ostringstream markerStream;
    markerStream << ipc::START_MARKER << paddedId << std::setw(8) << std::setfill('0') << messageLength; // NOLINT - magic numbers
    std::string start_marker = markerStream.str();

    std::string formattedRequest = start_marker + message;
    logger->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "..."); // NOLINT - magic numbers
    formattedRequest += "\n"; // add newline as a delimiter

    return formattedRequest;
}

// add request to queue
auto IPCQueue::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) -> void {
    // the pipe check is called when the request is actually written --
    // this just queues the request. But we shouldn't queue a request
    // if the pipes aren't initialized
    auto state = getState();
    if (state != ipc::QueueState::Running) {
        logger->error("IPCQueue not running. Cannot write request.");
        return;
    }

    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(message, callback);
    lock.unlock();
    logger->debug("Request enqueued: " + message);

    // If no request is currently being processed, start processing
    if (state == ipc::QueueState::Running && state != ipc::QueueState::Processing) {
        processNextRequest();
    }
}

auto IPCQueue::processNextRequest() -> void {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (stopIPC_) {
        logger->info("Stopping IPCQueue request processing.");
        setState(ipc::QueueState::Running);
        return;
    }

    if (requestQueue_.empty()) {
        setState(ipc::QueueState::Running);
        return;
    }

    setState(ipc::QueueState::Processing);

    auto nextRequest = requestQueue_.front();
    requestQueue_.pop();
    lock.unlock();

    logger->debug("Processing next request: " + nextRequest.first);
    std::thread([this, nextRequest]() {
        if (getState() != ipc::QueueState::Halted) {
            this->writeRequestInternal(nextRequest.first, nextRequest.second);
        }

        // Live operates on 100ms tick -- without this sleep commands are skipped
        std::this_thread::sleep_for(ipc::LIVE_TICK);

        if (getState() != ipc::QueueState::Halted) {
            this->processNextRequest();
        }
    }).detach();
}

auto IPCQueue::writeRequestInternal(const std::string& message, ipc::ResponseCallback callback) -> bool {
    std::string formattedRequest;
    uint64_t id = nextRequestId_++;
    try {
        formattedRequest = formatRequest(message, id);
        logger->debug("Request formatted successfully");
    } catch (const std::exception& e) {
        logger->error("Exception in formatRequest: " + std::string(e.what()));
        return false;
    }

    if (requestPipe_->writeToPipe(formattedRequest, callback)) {
        std::thread readerThread([this, callback]() {
            std::this_thread::sleep_for(ipc::LIVE_TICK * 2);
            responsePipe_->readResponse(callback);
        });
        readerThread.detach();
    }

    return true;
}

auto IPCQueue::createExtendedCallback(const ipc::ResponseCallback& originalCallback) -> ipc::ResponseCallback {
    return [this, originalCallback](const std::string& response, bool success) {
        // Call the original callback
        originalCallback(response, success);

        // Set the state of the IPCQueue
        if (success) {
            this->setState(ipc::QueueState::Running);
        } else {
            this->setState(ipc::QueueState::Error);
            // Optionally, you might want to initiate recovery here
            // this->initiateRecovery();
        }
    };
}


auto IPCQueue::cleanUpPipes() -> void {
    requestPipe_->cleanUp();
    responsePipe_->cleanUp();
}

void IPCQueue::stopIPC() {
    requestPipe_->stop();
    responsePipe_->stop();
}
