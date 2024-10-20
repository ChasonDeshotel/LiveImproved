#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCBase.h"

IPCBase::IPCBase()
    : IIPC()
    , isProcessingRequest_(false)
    , _requestPipeHandle_(INVALID_PIPE_HANDLE)
    , _responsePipeHandle_(INVALID_PIPE_HANDLE)
{
    auto path = PathManager();
    _requestPipePath_  = path.requestPipe().generic_string();
    _responsePipePath_ = path.responsePipe().generic_string();
}

IPCBase::~IPCBase() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }

    std::filesystem::remove(_requestPipePath_);
    std::filesystem::remove(_responsePipePath_);
}

auto IPCBase::init() -> bool {
    logger->debug("IPCBase::init() called");
    _stopIPC_ = false;

    std::thread createReadPipeThread(&IPCBase::createReadPipe, this);
    std::thread createWritePipeThread(&IPCBase::createWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(_createPipesMutex_);
        _createPipesCv_.wait(lock, [this] { return (_readPipeCreated_ && _writePipeCreated_) || _stopIPC_; });
    }

    createReadPipeThread.join();
    createWritePipeThread.join();

    if (!_readPipeCreated_ || !_writePipeCreated_) {
        logger->error("IPCBase::init() failed to create pipes");
        return false;
    }

    std::thread readyReadThread(&IPCBase::readyReadPipe, this);
    std::thread readyWriteThread(&IPCBase::readyWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] { return (_readPipeReady_ && _writePipeReady_) || _stopIPC_; });
    }

    readyReadThread.join();
    readyWriteThread.join();

    if (_readPipeReady_ && _writePipeReady_) {
        _isInitialized_ = true;
        logger->info("IPCBase::init() read/write enabled");
        return true;
    } else {
        logger->error("IPCBase::init() failed");
        return false;
    }
}

auto IPCBase::createReadPipeLoop() -> void {
    logger->debug("Creating read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (_stopIPC_) {
            logger->info("IPCBase read pipe creation cancelled.");
            return;
        }
        if (createReadPipe()) {
            // initialize the file descriptor -- indicates pipe has not yet been opened
            _responsePipeHandle_ = NULL_PIPE_HANDLE;
            logger->info("Response pipe successfully created");
            _readPipeCreated_ = true;
            _createPipesCv_.notify_one();
            return;
        }
        logger->warn("Attempt to create response pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    logger->error("Max attempts reached for creating response pipe");
}

auto IPCBase::createWritePipeLoop() -> void {
    logger->debug("Creating write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (_stopIPC_) {
            logger->info("IPCBase write pipe creation cancelled.");
            return;
        }
        if (createWritePipe()) {
            _requestPipeHandle_ = NULL_PIPE_HANDLE;
            logger->info("Request pipe successfully created");
            _writePipeCreated_ = true;
            _createPipesCv_.notify_one();
            return;
        }
        logger->warn("Attempt to create request pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    logger->error("Max attempts reached for creating request pipe");
}

auto IPCBase::readyReadPipe() -> void {
    logger->debug("Setting up read pipe. Path: " + _responsePipePath_.string());
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (_stopIPC_) {
            logger->warn("IPCBase read initialization cancelled.");
            return;
        }
        if (openPipeForRead(_responsePipePath_, true)) {
            logger->info("Response pipe successfully opened for reading");
            _readPipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
    return;
}

auto IPCBase::readyWritePipe() -> void {
    logger->debug("Setting up write pipe. Path: " + _requestPipePath_.string());
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (_stopIPC_) {
            logger->warn("IPCBase write initialization cancelled.");
            return;
        }
        if (openPipeForWrite(_requestPipePath_, true)) {
            logger->info("Request pipe successfully opened for writing");
            _writePipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        logger->warn("Attempt to open request pipe for writing failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening request pipe");
    return;
}

auto IPCBase::closeAndDeletePipes() -> void {
    cleanUpPipe(_requestPipePath_, _requestPipeHandle_);
    cleanUpPipe(_responsePipePath_, _responsePipeHandle_);
}

auto IPCBase::resetResponsePipe() -> void {
    logger->debug("Resetting response pipe");
    close(pipes_[_responsePipePath_]);
    pipes_[_responsePipePath_] = INVALID_PIPE_HANDLE;
    if (!openPipeForRead(_responsePipePath_, true)) {
        logger->error("Failed to reopen response pipe");
    } else {
        logger->info("Response pipe reopened successfully");
    }
}

auto IPCBase::removePipeIfExists(const std::string& pipeName) -> void {
    if (access(pipeName.c_str(), F_OK) != -1) {
        // File exists, so remove it
        if (unlink(pipeName.c_str()) == 0) {
            logger->debug("Removed existing pipe: " + pipeName);
        } else {
            logger->error("Failed to remove existing pipe: " + pipeName + " - " + strerror(errno));
        }
    }
}

auto IPCBase::openPipeForWrite(const std::string& pipeName, bool nonBlocking) -> bool {
    int flags = O_WRONLY;
    if (nonBlocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipeName.c_str(), flags); // NOLINT
    if (fd == -1) {
        logger->error("Failed to open pipe for writing: " + pipeName + " - " + strerror(errno));
        return false;
    }

    pipes_[pipeName] = fd;
    logger->debug("Pipe opened for writing: " + pipeName);
    return true;
}

auto IPCBase::openPipeForRead(const std::string& pipeName, bool nonBlocking) -> bool {
    int flags = O_RDONLY;
    if (nonBlocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipeName.c_str(), flags); // NOLINT
    if (fd == -1) {
        logger->error("Failed to open pipe for reading: " + pipeName + " - " + strerror(errno));
        return false;
    }

    pipes_[pipeName] = fd;
    logger->info("Pipe opened for reading: " + pipeName);
    return true;
}

auto IPCBase::formatRequest(const std::string& message, uint64_t id) -> std::string {
    size_t messageLength = message.length();

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000); // NOLINT
    std::string paddedId = idStream.str();

    std::ostringstream markerStream;
    markerStream << "START_" << paddedId << std::setw(8) << std::setfill('0') << messageLength; // NOLINT
    std::string start_marker = markerStream.str();

    std::string formattedRequest = start_marker + message;
    logger->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "..."); // NOLINT

    return formattedRequest;
}

// add request to queue
auto IPCBase::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) -> void {
    // the pipe check is called when the request is actually written --
    // this just queues the request. But we shouldn't queue a request
    // if the pipes aren't initialized
    if (!_isInitialized_) {
        logger->error("IPCBase not initialized. Cannot write request.");
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

auto IPCBase::processNextRequest() -> void {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (_stopIPC_) {
        logger->info("Stopping IPCBase request processing.");
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
        if (!_stopIPC_) {
            this->writeRequestInternal(nextRequest.first, nextRequest.second);
        }

        usleep(100000); // NOLINT Live operates on 100ms tick -- without this sleep commands are skipped

        if (!_stopIPC_) {
            this->processNextRequest();
        }
    }).detach();
}

auto IPCBase::writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool {
	// Check if the pipe is already open for writing
	if (pipes_[_requestPipePath_] == INVALID_PIPE_HANDLE) {
		if (!openPipeForWrite(_requestPipePath_, true)) {  // Open in non-blocking mode
			return false;
		}
	}
	int fd = pipes_[_requestPipePath_];
	if (fd == INVALID_PIPE_HANDLE) {
		logger->error("Request pipe not opened for writing: " + _requestPipePath_.string());
		return false;
	}
	drainPipe(pipes_[_responsePipePath_]);

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

	ssize_t result = write(fd, formattedRequest.c_str(), formattedRequest.length());

    ssize_t bytesWritten = write(pipes_[_requestPipePath_], formattedRequest.c_str(), formattedRequest.length());
	if (bytesWritten == -1) {
		if (errno == EAGAIN) {
			logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
		} else {
			logger->error("Failed to write to request pipe: " + _requestPipePath_.string() + " - " + strerror(errno));
		}
		return false;
	}

    if (bytesWritten != static_cast<ssize_t>(formattedRequest.size())) {
        logger->error("Failed to write the full request. Bytes written: " + std::to_string(bytesWritten));
        return false;
    }

    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));

    std::thread readerThread([this, callback]() {
        this->readResponse(callback);
    });
    readerThread.detach();

    return true;
}

auto IPCBase::drainPipe(int fd) -> void {
    std::array<char, BUFFER_SIZE> buffer{};

    // Use non-blocking read to drain the pipe
    ssize_t bytesRead = 0;
    do { // NOLINT - don't be stupid. Know and love the do-while
        bytesRead = read(fd, buffer.data(), buffer.size());  // Use .data() to get the pointer
    } while (bytesRead > 0);
}

auto IPCBase::readResponse(ResponseCallback callback) -> std::string {
    logger->debug("IPCBase::readResponse() called");

    int fd = pipes_[_responsePipePath_];

    if (fd == -1) {
        logger->error("Response pipe is not open for reading.");
        if (!openPipeForRead(_responsePipePath_, true)) {  // Open in non-blocking mode
            return "";
        }
        fd = pipes_[_responsePipePath_];  // Reassign fd after reopening the pipe
    }

    std::string requestId;

    const size_t START_MARKER_SIZE = 6;
    const size_t REQUEST_ID_SIZE = 8;
    const size_t HEADER_SIZE = START_MARKER_SIZE + REQUEST_ID_SIZE + 8;
    std::array<char, HEADER_SIZE + 1> header{}; // +1 for null termination
    ssize_t bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty or partial reads
    int retry_count = 0;
    while (totalHeaderRead < HEADER_SIZE && retry_count < MAX_READ_RETRIES) {
        if (_stopIPC_) {
            logger->info("IPCBase write initialization cancelled during read pipe setup.");
            return "";
        }
        auto startIt = header.begin() + totalHeaderRead;
        bytesRead = read(fd, &(*startIt), HEADER_SIZE - totalHeaderRead);

        logger->debug("Header partial read: " + std::string(header.data(), totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                usleep(DELAY_BETWEEN_READS);
                continue;
            } else {
                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
                return "";
            }
        }

        if (bytesRead == 0) {
            retry_count++;
            usleep(DELAY_BETWEEN_READS);
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
        if (_stopIPC_) {
            logger->info("IPCBase write initialization cancelled during read pipe setup.");
            return "";
        }
        size_t bytesToRead = std::min(BUFFER_SIZE, messageSize + END_MARKER.size() - totalBytesRead);
        ssize_t bytesRead = read(fd, buffer.data(), bytesToRead);

        if (bytesRead <= 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            usleep(DELAY_BETWEEN_READS);
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

    logger->debug("Message read from response pipe: " + _responsePipePath_.string());
    if (message.length() > MESSAGE_TRUNCATE_CHARS) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, MESSAGE_TRUNCATE_CHARS));
    } else {
        logger->debug("Message: " + message);
    }

    if (callback && !_stopIPC_) {
        callback(message);
    }

    return message;
}

 auto IPCBase::cleanUpPipe(const Path& path, PipeHandle& handle) -> void {
     // Default implementation or leave it pure virtual
     throw std::runtime_error("cleanUpPipe not implemented in base class");
 }

 auto IPCBase::cleanUpPipes() -> void {
     // Default implementation
     cleanUpPipe(_requestPipePath_, _requestPipeHandle_);
     cleanUpPipe(_responsePipePath_, _responsePipeHandle_);
 }

 auto IPCBase::createReadPipe() -> bool {
     // Default implementation or leave it pure virtual
     throw std::runtime_error("createReadPipe not implemented in base class");
 }

 auto IPCBase::createWritePipe() -> bool {
     // Default implementation or leave it pure virtual
     throw std::runtime_error("createWritePipe not implemented in base class");
 }
