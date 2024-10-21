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
            // initialize the file descriptor -- indicates pipe has not yet been opened
            responsePipeHandle_ = NULL_PIPE_HANDLE;
            logger->info("Response pipe successfully created");
            readPipeCreated_ = true;
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
            requestPipeHandle_ = NULL_PIPE_HANDLE;
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
    logger->debug("Setting up read pipe. Path: " + responsePipePath_.string());
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue read initialization cancelled.");
            return;
        }
        if (IPC::openResponsePipe(responsePipePath_, responsePipeHandle_)) {
            logger->info("Response pipe successfully opened for reading");
            readPipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
    return;
}

auto IPCQueue::readyWritePipe() -> void {
    logger->debug("Setting up write pipe. Path: " + requestPipePath_.string());
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue write initialization cancelled.");
            return;
        }
        if (IPC::openRequestPipe(requestPipePath_, requestPipeHandle_)) {
            logger->info("Request pipe successfully opened for writing");
            writePipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open request pipe for writing failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening request pipe");
    return;
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

// add request to queue
auto IPCQueue::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) -> void {
    // the pipe check is called when the request is actually written --
    // this just queues the request. But we shouldn't queue a request
    // if the pipes aren't initialized
    if (!isInitialized_) {
        logger->error("IPCQueue not initialized. Cannot write request.");
        return;
    }

    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(message, callback);
    lock.unlock();
    logger->debug("Request enqueued: " + message);

    // If no request is currently being processed, start processing
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

        // Live operates on 100ms tick -- without this sleep commands are skipped
        std::this_thread::sleep_for(LIVE_TICK);

        if (!stopIPC_) {
            this->processNextRequest();
        }
    }).detach();
}

auto IPCQueue::writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool {
	// Check if the pipe is already open for writing
	if (requestPipeHandle_ == INVALID_PIPE_HANDLE) {
		if (!IPC::openRequestPipe(requestPipePath_, requestPipeHandle_)) {
		    logger->error("Request pipe not opened for writing: " + requestPipePath_.string());
			return false;
		}
	}

    IPC::drainPipe(responsePipeHandle_, BUFFER_SIZE);

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

    // TODO add the delimiter on the formatter
    ssize_t bytesWritten = write(requestPipeHandle_, formattedRequest.c_str(), formattedRequest.length());
    if (bytesWritten == -1) {
        if (errno == EAGAIN) {
            logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            logger->error("Failed to write to request pipe: " + requestPipePath_.string() + " - " + strerror(errno));
        }
        return false;
    } else if (bytesWritten != formattedRequest.length()) {
        logger->error("Incomplete write to request pipe. Wrote " + std::to_string(bytesWritten) + " of " + std::to_string(formattedRequest.length()) + " bytes");
        return false;
    }
    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));

    std::thread readerThread([this, callback]() {
        std::this_thread::sleep_for(LIVE_TICK * 2);
        this->readResponse(callback);
    });
    readerThread.detach();

    return true;
}

auto IPCQueue::readResponse(ResponseCallback callback) -> std::string {
    logger->debug("IPCQueue::readResponse() called");

    int fd = responsePipeHandle_;

    if (fd == -1) {
        logger->error("Response pipe is not open for reading.");
        if (!IPC::openResponsePipe(responsePipePath_, responsePipeHandle_)) {  // Open in non-blocking mode
            return "";
        }
        fd = responsePipeHandle_;  // Reassign fd after reopening the pipe
    }

    std::string requestId;

    std::array<char, HEADER_SIZE + 1> header{}; // +1 for null termination
    ssize_t bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty or partial reads
    int retry_count = 0;
    while (totalHeaderRead < HEADER_SIZE && retry_count < MAX_READ_RETRIES) {
        if (stopIPC_) {
            logger->info("IPCQueue write initialization cancelled during read pipe setup.");
            return "";
        }
        auto startIt = header.begin() + totalHeaderRead;
        bytesRead = read(responsePipeHandle_, &(*startIt), HEADER_SIZE - totalHeaderRead);

        logger->debug("Header partial read: " + std::string(header.data(), totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                std::this_thread::sleep_for(DELAY_BETWEEN_READS);
                continue;
            } else {
                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
                return "";
            }
        }

        if (bytesRead == 0) {
            retry_count++;
            std::this_thread::sleep_for(DELAY_BETWEEN_READS);
            continue;
        }

        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != HEADER_SIZE) {
        logger->error("Failed to read the full header after " + std::to_string(retry_count) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
        return "";
    }

    logger->debug("Full header received: " + std::string(header.data(), totalHeaderRead));

    // size_t instead of int because comparisons
    size_t messageSize = 0;
    try {
        // Extract the response size (last 8 characters of the header)
        std::string messageSizeStr(header.data() + 14);  // NOLINT Skip 'START_' and the 8 characters of request ID
        messageSize = std::stoull(messageSizeStr);  // Convert to size_t
    } catch (const std::invalid_argument& e) {
        logger->error("Invalid header. Could not parse message size: " + std::string(e.what()));
        return "";
    } catch (const std::out_of_range& e) {
        logger->error("Header size out of range: " + std::string(e.what()));
        return "";
    }

    logger->debug("Message size to read: " + std::to_string(messageSize));

    // init to empty string for callbacks expecting a string arg
    std::string message = "";
    size_t totalBytesRead = 0;
    std::vector<char> buffer(BUFFER_SIZE);

    while (totalBytesRead < messageSize + END_MARKER.size()) {
        if (stopIPC_) {
            logger->info("IPCQueue write initialization cancelled during read pipe setup.");
            return "";
        }
        size_t bytesToRead = std::min(BUFFER_SIZE, messageSize + END_MARKER.size() - totalBytesRead);
        ssize_t bytesRead = read(responsePipeHandle_, buffer.data(), bytesToRead);

        if (bytesRead <= 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            std::this_thread::sleep_for(DELAY_BETWEEN_READS);
            continue;
        }

        message.append(buffer.data(), bytesRead);
        totalBytesRead += bytesRead;
        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // check for end marker in the accumulated message
        if (message.size() >= END_MARKER.size()) {
            if (message.compare(message.size() - END_MARKER.size(), END_MARKER.size(), END_MARKER) == 0) {
                logger->debug("End of message marker found.");
                message = message.substr(0, message.size() - END_MARKER.size()); // Remove the end marker
                break;
            }
        }
    }

    logger->debug("Total bytes read: " + std::to_string(totalBytesRead - END_MARKER.size()));

    logger->debug("Message read from response pipe: " + responsePipePath_.string());
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
    IPC::cleanUpPipe(requestPipePath_, requestPipeHandle_);
    IPC::cleanUpPipe(responsePipePath_, responsePipeHandle_);
}

auto IPCQueue::createReadPipe() -> bool {
    return IPC::createPipe(responsePipePath_);
}

auto IPCQueue::createWritePipe() -> bool {
    return IPC::createPipe(requestPipePath_);
}

