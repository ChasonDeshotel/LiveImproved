#include "IPC.h"
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

#include "ILogHandler.h"
#include "IPC.h"

IPC::IPC(
    std::function<std::shared_ptr<ILogHandler>()> logHandler
)
    : IIPC()
    , logHandler_(std::move(logHandler))
    , isProcessingRequest_(false) {
    if (!logHandler_()) {
        throw std::invalid_argument("IPC requires valid logHandler");
    }
}

IPC::~IPC() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }
    std::filesystem::remove(requestPipePath);
    std::filesystem::remove(responsePipePath);
}

bool IPC::init() {
    log_()->debug("IPC::init() called");
    stopIPC_ = false;

    std::thread createReadPipeThread(&IPC::createReadPipe, this);
    std::thread createWritePipeThread(&IPC::createWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(createPipesMutex_);
        createPipesCv_.wait(lock, [this] { return (readPipeCreated_ && writePipeCreated_) || stopIPC_; });
    }

    createReadPipeThread.join();
    createWritePipeThread.join();

    if (!readPipeCreated_ || !writePipeCreated_) {
        log_()->error("IPC::init() failed to create pipes");
        return false;
    }

    std::thread readyReadThread(&IPC::readyReadPipe, this);
    std::thread readyWriteThread(&IPC::readyWritePipe, this);

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        initCv_.wait(lock, [this] { return (readPipeReady_ && writePipeReady_) || stopIPC_; });
    }

    readyReadThread.join();
    readyWriteThread.join();

    if (readPipeReady_ && writePipeReady_) {
        isInitialized_ = true;
        log_()->info("IPC::init() read/write enabled");
        return true;
    } else {
        log_()->error("IPC::init() failed");
        return false;
    }
}

void IPC::readyReadPipe() {
    log_()->debug("Setting up read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            log_()->warn("IPC read initialization cancelled.");
            return;
        }
        if (openPipeForRead(responsePipePath, true)) {
            log_()->info("Response pipe successfully opened for reading");
            readPipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        log_()->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    log_()->error("Max attempts reached for opening response pipe");
    return;
}

void IPC::readyWritePipe() {
    log_()->debug("Setting up write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            log_()->warn("IPC write initialization cancelled.");
            return;
        }
        if (openPipeForWrite(requestPipePath, true)) {
            log_()->info("Request pipe successfully opened for writing");
            writePipeReady_.store(true, std::memory_order_release);
            initCv_.notify_one();
            return;
        }
        log_()->warn("Attempt to open request pipe for writing failed. Retrying...");
        std::this_thread::sleep_for(PIPE_SETUP_RETRY_DELAY);
    }
    log_()->error("Max attempts reached for opening request pipe");
    return;
}

void IPC::closeAndDeletePipes() {
    for (auto& [pipeName, pipeFD] : pipes_) {
        if (pipeFD != -1) {
            close(pipeFD);
            pipeFD = -1;  // Reset the file descriptor after closing
        }
    }
    std::filesystem::remove(requestPipePath);
    std::filesystem::remove(responsePipePath);
}

void IPC::resetResponsePipe() {
    log_()->debug("Resetting response pipe");
    close(pipes_[responsePipePath]);
    pipes_[responsePipePath] = -1;
    if (!openPipeForRead(responsePipePath, true)) {
        log_()->error("Failed to reopen response pipe");
    } else {
        log_()->info("Response pipe reopened successfully");
    }
}

void IPC::removePipeIfExists(const std::string& pipe_name) {
    if (access(pipe_name.c_str(), F_OK) != -1) {
        // File exists, so remove it
        if (unlink(pipe_name.c_str()) == 0) {
            log_()->debug("Removed existing pipe: " + pipe_name);
        } else {
            log_()->error("Failed to remove existing pipe: " + pipe_name + " - " + strerror(errno));
        }
    }
}

void IPC::createReadPipe() {
    log_()->debug("Creating read pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            log_()->info("IPC read pipe creation cancelled.");
            return;
        }
        if (createPipe(responsePipePath)) {
            log_()->info("Response pipe successfully created");
            readPipeCreated_ = true;
            createPipesCv_.notify_one();
            return;
        }
        log_()->warn("Attempt to create response pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    log_()->error("Max attempts reached for creating response pipe");
}

void IPC::createWritePipe() {
    log_()->debug("Creating write pipe");
    for (int attempt = 0; attempt < MAX_PIPE_CREATION_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            log_()->info("IPC write pipe creation cancelled.");
            return;
        }
        if (createPipe(requestPipePath)) {
            log_()->info("Request pipe successfully created");
            writePipeCreated_ = true;
            createPipesCv_.notify_one();
            return;
        }
        log_()->warn("Attempt to create request pipe failed. Retrying...");
        std::this_thread::sleep_for(PIPE_CREATION_RETRY_DELAY);
    }
    log_()->error("Max attempts reached for creating request pipe");
}

bool IPC::createPipe(const std::string& pipe_name) {
    // Extract the directory path from the pipe_name
    std::string directory = pipe_name.substr(0, pipe_name.find_last_of('/'));

    // Check if the directory exists, and create it if it doesn't
    struct stat st;
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), 0777) == -1) {
            log_()->error("Failed to create directory: " + directory + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipe_name.c_str(), 0666) == -1) {
        if (errno == EEXIST) {
            log_()->warn("Pipe already exists: " + pipe_name);
        } else {
            log_()->error("Failed to create named pipe: " + pipe_name + " - " + strerror(errno));
            return false;
        }
    } else {
        log_()->debug("Pipe created: " + pipe_name);
    }

    // Init file descriptor -- indicates pipe has not yet been opened
    pipes_[pipe_name] = -1;
    return true;
}

bool IPC::openPipeForWrite(const std::string& pipe_name, bool non_blocking) {
    int flags = O_WRONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        log_()->error("Failed to open pipe for writing: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    log_()->debug("Pipe opened for writing: " + pipe_name);
    return true;
}

bool IPC::openPipeForRead(const std::string& pipe_name, bool non_blocking) {
    int flags = O_RDONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        log_()->error("Failed to open pipe for reading: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    log_()->info("Pipe opened for reading: " + pipe_name);
    return true;
}

std::string IPC::formatRequest(const std::string& message, uint64_t id) {
    size_t messageLength = message.length();

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000);
    std::string paddedId = idStream.str();

    std::ostringstream markerStream;
    markerStream << "START_" << paddedId << std::setw(8) << std::setfill('0') << messageLength;
    std::string start_marker = markerStream.str();

    std::string formattedRequest = start_marker + message;
    log_()->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "...");

    return formattedRequest;
}

// add request to queue
void IPC::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) {
    // the pipe check is called when the request is actually written --
    // this just queues the request. But we shouldn't queue a request
    // if the pipes aren't initialized
    if (!isInitialized_) {
        log_()->error("IPC not initialized. Cannot write request.");
        return;
    }

    std::unique_lock<std::mutex> lock(queueMutex_);
    requestQueue_.emplace(message, callback);
    lock.unlock();
    log_()->debug("Request enqueued: " + message);

    // If no request is currently being processed, start processing
    if (!isProcessingRequest_) {
        processNextRequest();
    }
}

void IPC::processNextRequest() {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (stopIPC_) {
        log_()->info("Stopping IPC request processing.");
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

    log_()->debug("Processing next request: " + nextRequest.first);
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

bool IPC::writeRequestInternal(const std::string& message, ResponseCallback callback) {
	// Check if the pipe is already open for writing
	if (pipes_[requestPipePath] == -1) {
		if (!openPipeForWrite(requestPipePath, true)) {  // Open in non-blocking mode
			return false;
		}
	}
	int fd = pipes_[requestPipePath];
	if (fd == -1) {
		log_()->error("Request pipe not opened for writing: " + requestPipePath);
		return false;
	}
	drainPipe(pipes_[responsePipePath]);

    uint64_t id = nextRequestId_++;

    std::string formattedRequest;
    try {
        formattedRequest = formatRequest(message, id);
        log_()->debug("Request formatted successfully");
    } catch (const std::exception& e) {
        log_()->error("Exception in formatRequest: " + std::string(e.what()));
        return false;
    }

    log_()->debug("Writing request: " + formattedRequest);

	ssize_t result = write(fd, formattedRequest.c_str(), formattedRequest.length());

    ssize_t bytesWritten = write(pipes_[requestPipePath], formattedRequest.c_str(), formattedRequest.length());
	if (bytesWritten == -1) {
		if (errno == EAGAIN) {
			log_()->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
		} else {
			log_()->error("Failed to write to request pipe: " + requestPipePath + " - " + strerror(errno));
		}
		return false;
	}

    if (bytesWritten != static_cast<ssize_t>(formattedRequest.size())) {
        log_()->error("Failed to write the full request. Bytes written: " + std::to_string(bytesWritten));
        return false;
    }

    log_()->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));

    std::thread readerThread([this, callback]() {
        this->readResponse(callback);
    });
    readerThread.detach();

    return true;
}

void IPC::drainPipe(int fd) {
    const size_t bufferSize = 4096;
    char buffer[bufferSize];

    // Use non-blocking read to drain the pipe
    ssize_t bytesRead = 0;
    do {
        bytesRead = read(fd, buffer, bufferSize);
    } while (bytesRead > 0);
}

std::string IPC::readResponse(ResponseCallback callback) {
    log_()->debug("IPC::readResponse() called");

    int fd = pipes_[responsePipePath];

    if (fd == -1) {
        log_()->error("Response pipe is not open for reading.");
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
            log_()->info("IPC write initialization cancelled during read pipe setup.");
            return "";
        }
        bytesRead = read(fd, header + totalHeaderRead, HEADER_SIZE - totalHeaderRead);

        log_()->debug("Header partial read: " + std::string(header, totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                usleep(20000);
                continue;
            } else {
                log_()->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
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
        log_()->error("Failed to read the full header after " + std::to_string(retry_count) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
        return "";
    }

    header[HEADER_SIZE] = '\0';  // Null-terminate the header string
    log_()->debug("Full header received: " + std::string(header));

    // Convert the header to an integer representing the message size
    size_t messageSize;
    try {
        // Extract the response size (last 8 characters of the header)
        std::string messageSizeStr(header + 14);  // Skip 'START_' and the 8 characters of request ID
        messageSize = std::stoull(messageSizeStr);  // Convert to size_t
    } catch (const std::invalid_argument& e) {
        log_()->error("Invalid header. Could not parse message size: " + std::string(e.what()));
        return "";
    } catch (const std::out_of_range& e) {
        log_()->error("Header size out of range: " + std::string(e.what()));
        return "";
    }

    log_()->debug("Message size to read: " + std::to_string(messageSize));

    std::string message;
    size_t totalBytesRead = 0;
    const size_t bufferSize = 8192;
    char buffer[bufferSize];

    while (totalBytesRead < messageSize + endMarker.size()) {
        if (stopIPC_) {
            log_()->info("IPC write initialization cancelled during read pipe setup.");
            return "";
        }
        size_t bytesToRead = std::min(bufferSize, messageSize + endMarker.size() - totalBytesRead);
        bytesRead = read(fd, buffer, bytesToRead);

        if (bytesRead <= 0) {
            log_()->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            usleep(20000);
            continue;
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
        log_()->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // Check if the end marker is present in the accumulated message
        if (message.size() >= endMarker.size()) {
            if (message.compare(message.size() - endMarker.size(), endMarker.size(), endMarker) == 0) {
                log_()->debug("End of message marker found.");
                message = message.substr(0, message.size() - endMarker.size()); // Remove the end marker
                break;
            }
        }
    }

    log_()->debug("Total bytes read: " + std::to_string(totalBytesRead - endMarker.size()));

    log_()->debug("Message read from response pipe: " + responsePipePath);
    if (message.length() > 100) {
        log_()->debug("Message truncated to 100 characters");
        log_()->debug("Message: " + message.substr(0, 100));
    } else {
        log_()->debug("Message: " + message);
    }

    if (callback && !stopIPC_) {
        callback(message);
    }

    return message;
}
