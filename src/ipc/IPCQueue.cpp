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
    this->stopProcessing();
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
        startProcessing();
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

void IPCQueue::startProcessing() {
    processingThread_ = std::thread(&IPCQueue::processQueue, this);
}

void IPCQueue::stopProcessing() {
    stopIPC_ = true;
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void IPCQueue::processQueue() {
    while (!stopIPC_) {

        std::unique_lock<std::mutex> lock(queueMutex_);
        if (!requestQueue_.empty()) {
            auto request = std::move(requestQueue_.front());
            requestQueue_.pop();
            lock.unlock();

            setState(ipc::QueueState::Processing);
            logger->error("launching process request: " + request.message);
            processRequest(request);
            logger->error("finished process request: " + request.message);
        } else {
            lock.unlock();
        }
        std::this_thread::sleep_for(ipc::LIVE_TICK * 2);
    }
}

auto IPCQueue::createRequest(const std::string& message, ipc::ResponseCallback callback = [](const std::string&) {}) -> void {
    ipc::Request req(message, callback);
    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(req);
    lock.unlock();
    logger->error("Request enqueued: " + req.message);
    logger->debug("New request added. Queue size: " + std::to_string(requestQueue_.size()));
}


void IPCQueue::processRequest(const ipc::Request& request) {
    logger->error("processing: " + request.message);
    if (getState() != ipc::QueueState::Halted) {
        writeToPipe(request);
    }
}

auto IPCQueue::processNextRequest() -> void {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (stopIPC_) {
        logger->info("Stopping IPCQueue request processing.");
        setState(ipc::QueueState::Halted);
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

    logger->debug("Processing next request: " + nextRequest.message);
    std::thread([this, nextRequest]() {
        if (getState() != ipc::QueueState::Halted) {
            this->writeToPipe(nextRequest);
        }

        // Live operates on 100ms tick -- without this sleep commands are skipped
        std::this_thread::sleep_for(ipc::LIVE_TICK);

        if (getState() != ipc::QueueState::Halted) {
            this->processNextRequest();
        }
    }).detach();
}

auto IPCQueue::writeToPipe(ipc::Request request) -> bool {
    std::string formattedRequest;
    uint64_t id = nextRequestId_++;

    if (requestPipe_->writeToPipe(request)) {
        std::thread readerThread([this, request]() {
            std::this_thread::sleep_for(ipc::LIVE_TICK * 2);
            auto response = responsePipe_->readResponse(request.callback);
            if (response.type == ipc::ResponseType::Success) {
                logger->error("got success.");
                setState(ipc::QueueState::Running);
            } else {
                // TODO: make halt method
                logger->error("not success.");
                setState(ipc::QueueState::Halted);
            }
        });
        readerThread.detach();
    }

    return true;
}

auto IPCQueue::cleanUpPipes() -> void {
    requestPipe_->cleanUp();
    responsePipe_->cleanUp();
}

void IPCQueue::stopIPC() {
    requestPipe_->stop();
    responsePipe_->stop();
}
