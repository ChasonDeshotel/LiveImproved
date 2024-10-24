#include <fcntl.h>
#include <mutex>
#include <thread>

#include "LogGlobal.h"

#include "IPCDefinitions.h"
#include "IPCQueue.h"
#include "IPCRequestPipe.h"
#include "IPCResponsePipe.h"
#include "PipeUtil.h"

IPCQueue::IPCQueue(
         std::function<std::shared_ptr<IPCRequestPipe>()> requestPipe
         , std::function<std::shared_ptr<IPCResponsePipe>()> responsePipe
        )
    : IIPCQueue()
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

auto IPCQueue::init() -> ipc::QueueState {
    logger->debug("IPCQueue::init() called");

    // create the pipes
    if(!requestPipe_->getPipeUtil()->create() || !responsePipe_->getPipeUtil()->create()) {
        logger->error("IPCQueue::init() failed to create pipes");
    }

    std::thread readyRequestThread(&IPCQueue::readyRequestWrapper, this);
    std::thread readyResponseThread(&IPCQueue::readyResponseWrapper, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] {
                return (requestPipeReady_ && responsePipeReady_)
                        || getState() == ipc::QueueState::Halted;
                });
    }

    readyRequestThread.join();
    readyResponseThread.join();

    if ((requestPipeReady_ && responsePipeReady_) && getState() != ipc::QueueState::Halted) {
        logger->info("IPCQueue::init() read/write enabled");
        setState(ipc::QueueState::Running);
        startProcessing();
        return ipc::QueueState::Running;
    } else {
        logger->error("IPCQueue::init() failed");
        setState(ipc::QueueState::Halted);
        return ipc::QueueState::Halted;
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

void IPCQueue::halt() {
    stopProcessing();
    requestPipe_->stop();
    responsePipe_->stop();
}

void IPCQueue::stopProcessing() {
    setState(ipc::QueueState::Halted);
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void IPCQueue::processQueue() {
    while (getState() != ipc::QueueState::Halted) {

        std::unique_lock<std::mutex> lock(queueMutex_);
        if (!requestQueue_.empty()) {
            auto request = std::move(requestQueue_.front());
            requestQueue_.pop();
            lock.unlock();

            setState(ipc::QueueState::Processing);
            processRequest(request);
        } else {
            lock.unlock();
        }
        std::this_thread::sleep_for(ipc::LIVE_TICK);
    }
}

auto IPCQueue::createRequest(const std::string& message, ipc::ResponseCallback callback = [](const std::string&) {}) -> void {
    ipc::Request req(message, callback);
    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(req);
    lock.unlock();
    logger->debug("Request enqueued: " + req.message + " Queue size: " + std::to_string(requestQueue_.size()));
}


void IPCQueue::processRequest(const ipc::Request& request) {
    logger->error("processing: " + request.message);
    if (getState() != ipc::QueueState::Halted) {
        pipeObjWrite(request);
    }
}

auto IPCQueue::pipeObjWrite(ipc::Request request) -> bool {
    std::string formattedRequest;
    uint64_t id = nextRequestId_++;

    if (requestPipe_->writeRequest(request)) {
        std::thread readerThread([this, request]() {
            std::this_thread::sleep_for(ipc::LIVE_TICK * 2);
            auto response = responsePipe_->readResponse(request.callback);
            if (response.success()) {
                // done processing -- set queue back to running
                setState(ipc::QueueState::Running);
            } else {
                // TODO: make halt method
                logger->error("readResponse did not return success.");
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
