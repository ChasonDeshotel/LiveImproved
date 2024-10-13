#include "IPCCore.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <dispatch/dispatch.h>
#include <queue>
#include <map>
#include <thread>
#include <sstream>
#include <errno.h>
#include <cstring>

#include "LogGlobal.h"
#include "IPCCore.h"

IPCCore::IPCCore()
    : IIPCCore()
    , isProcessingRequest_(false)
{}

IPCCore::~IPCCore() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }
    std::filesystem::remove(requestPipePath);
    std::filesystem::remove(responsePipePath);
}

bool IPCCore::init() {
    logger->debug("IPCCore::init() called");
    stopIPC_ = false;

    std::thread createReadPipeThread(&IPCCore::createReadPipe, this);
    std::thread createWritePipeThread(&IPCCore::createWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(createPipesMutex_);
        createPipesCv_.wait(lock, [this] { return (readPipeCreated_ && writePipeCreated_) || stopIPC_; });
    }

    createReadPipeThread.join();
    createWritePipeThread.join();

    if (!readPipeCreated_ || !writePipeCreated_) {
        logger->error("IPCCore::init() failed to create pipes");
        return false;
    }

    std::thread readyReadThread(&IPCCore::readyReadPipe, this);
    std::thread readyWriteThread(&IPCCore::readyWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] { return (readPipeReady_ && writePipeReady_) || stopIPC_; });
    }

    readyReadThread.join();
    readyWriteThread.join();

    if (readPipeReady_ && writePipeReady_) {
        isInitialized_ = true;
        logger->info("IPCCore::init() read/write enabled");
        return true;
    } else {
        logger->error("IPCCore::init() failed");
        return false;
    }
}

void IPCCore::readyReadPipe() {
    logger->debug("Setting up read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPC read initialization cancelled.");
            return;
        }
        if (openPipeForRead(responsePipePath, true)) {
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

void IPCCore::readyWritePipe() {
    logger->debug("Setting up write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPC write initialization cancelled.");
            return;
        }
        if (openPipeForWrite(requestPipePath, true)) {
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

void IPCCore::closeAndDeletePipes() {
    for (auto& [pipeName, pipeFD] : pipes_) {
        if (pipeFD != -1) {
            close(pipeFD);
            pipeFD = -1;  // Reset the file descriptor after closing
        }
    }
    std::filesystem::remove(requestPipePath);
    std::filesystem::remove(responsePipePath);
}

void IPCCore::resetResponsePipe() {
    logger->debug("Resetting response pipe");
    close(pipes_[responsePipePath]);
    pipes_[responsePipePath] = -1;
    if (!openPipeForRead(responsePipePath, true)) {
        logger->error("Failed to reopen response pipe");
    } else {
        logger->info("Response pipe reopened successfully");
    }
}

void IPCCore::removePipeIfExists(const std::string& pipe_name) {
    if (access(pipe_name.c_str(), F_OK) != -1) {
        // File exists, so remove it
        if (unlink(pipe_name.c_str()) == 0) {
            logger->debug("Removed existing pipe: " + pipe_name);
        } else {
            logger->error("Failed to remove existing pipe: " + pipe_name + " - " + strerror(errno));
        }
    }
}

void IPCCore::createReadPipe() {
    logger->debug("Creating read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->info("IPC read pipe creation cancelled.");
            return;
        }
        if (createPipe(responsePipePath)) {
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

void IPCCore::createWritePipe() {
    logger->debug("Creating write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->info("IPC write pipe creation cancelled.");
            return;
        }
        if (createPipe(requestPipePath)) {
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

bool IPCCore::createPipe(const std::string& pipe_name) {
    // Extract the directory path from the pipe_name
    std::string directory = pipe_name.substr(0, pipe_name.find_last_of('/'));

    // Check if the directory exists, and create it if it doesn't
    struct stat st;
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), 0777) == -1) {
            logger->error("Failed to create directory: " + directory + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipe_name.c_str(), 0666) == -1) {
        if (errno == EEXIST) {
            logger->warn("Pipe already exists: " + pipe_name);
        } else {
            logger->error("Failed to create named pipe: " + pipe_name + " - " + strerror(errno));
            return false;
        }
    } else {
        logger->debug("Pipe created: " + pipe_name);
    }

    // Init file descriptor -- indicates pipe has not yet been opened
    pipes_[pipe_name] = -1;
    return true;
}

bool IPCCore::openPipeForWrite(const std::string& pipe_name, bool non_blocking) {
    int flags = O_WRONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        logger->error("Failed to open pipe for writing: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    logger->debug("Pipe opened for writing: " + pipe_name);
    return true;
}

bool IPCCore::openPipeForRead(const std::string& pipe_name, bool non_blocking) {
    int flags = O_RDONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        logger->error("Failed to open pipe for reading: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    logger->info("Pipe opened for reading: " + pipe_name);
    return true;
}

std::string IPCCore::formatRequest(const std::string& message, uint64_t id) {
    size_t messageLength = message.length();

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000);
    std::string paddedId = idStream.str();

    std::ostringstream markerStream;
    markerStream << "START_" << paddedId << std::setw(8) << std::setfill('0') << messageLength;
    std::string start_marker = markerStream.str();

    std::string formattedRequest = start_marker + message;
    logger->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "...");

    return formattedRequest;
}

// add request to queue
void IPCCore::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) {
    // the pipe check is called when the request is actually written --
    // this just queues the request. But we shouldn't queue a request
    // if the pipes aren't initialized
    if (!isInitialized_) {
        logger->error("IPC not initialized. Cannot write request.");
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

void IPCCore::processNextRequest() {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (stopIPC_) {
        logger->info("Stopping IPC request processing.");
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

        usleep(100000); // Live operates on 100ms tick -- without this sleep commands are skipped

        if (!stopIPC_) {
            this->processNextRequest();
        }
    }).detach();
}

bool IPCCore::writeRequestInternal(const std::string& message, ResponseCallback callback) {
	// Check if the pipe is already open for writing
	if (pipes_[requestPipePath] == -1) {
		if (!openPipeForWrite(requestPipePath, true)) {  // Open in non-blocking mode
			return false;
		}
	}
	int fd = pipes_[requestPipePath];
	if (fd == -1) {
		logger->error("Request pipe not opened for writing: " + requestPipePath);
		return false;
	}
	drainPipe(pipes_[responsePipePath]);

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

    ssize_t bytesWritten = write(pipes_[requestPipePath], formattedRequest.c_str(), formattedRequest.length());
	if (bytesWritten == -1) {
		if (errno == EAGAIN) {
			logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
		} else {
			logger->error("Failed to write to request pipe: " + requestPipePath + " - " + strerror(errno));
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

void IPCCore::drainPipe(int fd) {
    const size_t bufferSize = 4096;
    char buffer[bufferSize];

    // Use non-blocking read to drain the pipe
    ssize_t bytesRead = 0;
    do {
        bytesRead = read(fd, buffer, bufferSize);
    } while (bytesRead > 0);
}

std::string IPCCore::readResponse(ResponseCallback callback) {
    logger->debug("IPCCore::readResponse() called");

    int fd = pipes_[responsePipePath];

    if (fd == -1) {
        logger->error("Response pipe is not open for reading.");
        if (!openPipeForRead(responsePipePath, true)) {  // Open in non-blocking mode
            return "";
        }
        fd = pipes_[responsePipePath];  // Reassign fd after reopening the pipe
    }

    const std::string startMarker = "START_";
    const std::string endMarker = "END_OF_MESSAGE";
    std::string requestId;

    const size_t START_MARKER_SIZE = 6;
    const size_t REQUEST_ID_SIZE = 8;
    const size_t HEADER_SIZE = START_MARKER_SIZE + REQUEST_ID_SIZE + 8;
    char header[HEADER_SIZE + 1];   // +1 for null-termination

    ssize_t bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty or partial reads
    int retry_count = 0;
    int max_retries = 100;
    while (totalHeaderRead < HEADER_SIZE && retry_count < max_retries) {
        if (stopIPC_) {
            logger->info("IPC write initialization cancelled during read pipe setup.");
            return "";
        }
        bytesRead = read(fd, header + totalHeaderRead, HEADER_SIZE - totalHeaderRead);

        logger->debug("Header partial read: " + std::string(header, totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                usleep(20000);
                continue;
            } else {
                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
                return "";
            }
        }

        if (bytesRead == 0) {
            retry_count++;
            usleep(20000);
            continue;
        }

        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != HEADER_SIZE) {
        logger->error("Failed to read the full header after " + std::to_string(retry_count) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
        return "";
    }

    header[HEADER_SIZE] = '\0';  // Null-terminate the header string
    logger->debug("Full header received: " + std::string(header));

    // size_t instead of int because comparisons
    size_t messageSize;
    try {
        // Extract the response size (last 8 characters of the header)
        std::string messageSizeStr(header + 14);  // Skip 'START_' and the 8 characters of request ID
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
    const size_t bufferSize = 8192;
    char buffer[bufferSize];

    while (totalBytesRead < messageSize + endMarker.size()) {
        if (stopIPC_) {
            logger->info("IPC write initialization cancelled during read pipe setup.");
            return "";
        }
        size_t bytesToRead = std::min(bufferSize, messageSize + endMarker.size() - totalBytesRead);
        bytesRead = read(fd, buffer, bytesToRead);

        if (bytesRead <= 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            usleep(20000);
            continue;
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // check for end marker in the accumulated message
        if (message.size() >= endMarker.size()) {
            if (message.compare(message.size() - endMarker.size(), endMarker.size(), endMarker) == 0) {
                logger->debug("End of message marker found.");
                message = message.substr(0, message.size() - endMarker.size()); // Remove the end marker
                break;
            }
        }
    }

    logger->debug("Total bytes read: " + std::to_string(totalBytesRead - endMarker.size()));

    logger->debug("Message read from response pipe: " + responsePipePath);
    if (message.length() > 100) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, 100));
    } else {
        logger->debug("Message: " + message);
    }

    if (callback && !stopIPC_) {
        callback(message);
    }

    return message;
}
