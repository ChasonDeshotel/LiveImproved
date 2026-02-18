#include <cerrno>
#include <cstring>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathFinder.h"

#include "IPCCore.h"

IPCCore::~IPCCore() {}

void IPCCore::destroy() {
    stopIPC_ = true;
    
    if (clientFd_ != -1) {
        shutdown(clientFd_, SHUT_RDWR);
        close(clientFd_);
        clientFd_ = -1;
    }
    
    if (acceptThread_.joinable()) acceptThread_.join();
    if (readThread_.joinable()) readThread_.join();
}

// IPCCore.cpp
auto IPCCore::init() -> bool {
    if (serverFd_ != -1) {
        logger->info("IPCCore already initialized, skipping");
        return true;
    }
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(47474);

    if (bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        logger->error("Failed to bind: " + std::string(strerror(errno)));
        return false;
    }
    
    listen(serverFd_, 1);
    logger->info("Listening on port " + std::to_string(PORT));

    acceptThread_ = std::thread([this]() {
        clientFd_ = accept(serverFd_, nullptr, nullptr);
        if (clientFd_ < 0) return;
        logger->info("Python remote script connected");
        isInitialized_ = true;
        
        // start read loop once connected
        readThread_ = std::thread(&IPCCore::readLoop, this);
        readThread_.detach();
    });
    acceptThread_.detach();

    logger->info("IPCCore::init() read/write enabled");
    return true;
}

auto IPCCore::readLoop() -> void {
    std::array<char, BUFFER_SIZE> chunk{};
    
    while (!stopIPC_) {
        std::string buffer;
        
        while (!stopIPC_) {
            ssize_t bytesRead = recv(clientFd_, chunk.data(), chunk.size(), 0);
            
            if (bytesRead <= 0) {
                logger->warn("Client disconnected, waiting for reconnect...");
                close(clientFd_);
                clientFd_ = -1;
                isInitialized_ = false;
                break;
            }
            
            buffer.append(chunk.data(), bytesRead);

            logger->info("received: {}", chunk.data());
            
            size_t endPos = buffer.find("END_OF_MESSAGE");
            if (endPos == std::string::npos) continue; // not done yet
            
            size_t startPos = buffer.find("START_");
            if (startPos == std::string::npos) {
                buffer.clear();
                continue;
            }
            
            std::string fullMessage = buffer.substr(startPos, endPos - startPos);
            buffer.erase(0, endPos + 14); // 14 = len("END_OF_MESSAGE")
            
            {
                std::lock_guard<std::mutex> lock(responseMutex_);
                responseQueue_.push(fullMessage);
            }
            responseCv_.notify_one();
        }
        
        if (stopIPC_) break;
        
        logger->info("Waiting for reconnect...");
        clientFd_ = accept(serverFd_, nullptr, nullptr);
        if (clientFd_ < 0) {
            if (errno == EINVAL || stopIPC_) break;
            logger->error("Accept failed: " + std::string(strerror(errno)));
            continue;
        }
        logger->info("Client reconnected");
    }
}

auto IPCCore::readResponse(ResponseCallback callback) -> std::string {
    std::unique_lock<std::mutex> lock(responseMutex_);
    responseCv_.wait(lock, [this] { 
        return !responseQueue_.empty() || stopIPC_; 
    });
    
    if (stopIPC_ || responseQueue_.empty()) return "";
    
    std::string message = responseQueue_.front();
    responseQueue_.pop();
    lock.unlock();
    
    if (callback) callback(message);
    return message;
}

// add request to queue
auto IPCCore::writeRequest(const std::string& message, ResponseCallback callback = [](const std::string&) {}) -> void {
    if (!isInitialized_) {
        logger->error("IPC not initialized. Cannot write request.");
        return;
    }
    // just write directly, no chunking, no queue needed for simple case
    std::string formatted = formatRequest(message, nextRequestId_++);
    send(clientFd_, formatted.c_str(), formatted.length(), 0);
    // read response in thread
    std::thread([this, callback]() { readResponse(callback); }).detach();
}

auto IPCCore::formatRequest(const std::string& message, uint64_t id) -> std::string {
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

//auto IPCCore::processNextRequest() -> void {
//    std::unique_lock<std::mutex> lock(queueMutex_);
//
//    if (stopIPC_) {
//        logger->info("Stopping IPC request processing.");
//        isProcessingRequest_ = false;
//        return;
//    }
//
//    if (requestQueue_.empty()) {
//        isProcessingRequest_ = false;
//        return;
//    }
//
//    isProcessingRequest_ = true;
//
//    auto nextRequest = requestQueue_.front();
//    requestQueue_.pop();
//    lock.unlock();
//
//    logger->debug("Processing next request: " + nextRequest.first);
//    std::thread([this, nextRequest]() {
//        if (!stopIPC_) {
//            this->writeRequestInternal(nextRequest.first, nextRequest.second);
//        }
//
//        usleep(100000); // NOLINT Live operates on 100ms tick -- without this sleep commands are skipped
//
//        if (!stopIPC_) {
//            this->processNextRequest();
//        }
//    }).detach();
//}

//auto IPCCore::writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool {
//	// Check if the pipe is already open for writing
//	if (pipes_[requestPipePath_] == -1) {
//		if (!openPipeForWrite(requestPipePath_, true)) {  // Open in non-blocking mode
//			return false;
//		}
//	}
//	int fd = pipes_[requestPipePath_];
//	if (fd == -1) {
//		logger->error("Request pipe not opened for writing: " + requestPipePath_);
//		return false;
//	}
//	drainPipe(pipes_[responsePipePath_]);
//
//    uint64_t id = nextRequestId_++;
//
//    std::string formattedRequest;
//    try {
//        formattedRequest = formatRequest(message, id);
//        logger->debug("Request formatted successfully");
//    } catch (const std::exception& e) {
//        logger->error("Exception in formatRequest: " + std::string(e.what()));
//        return false;
//    }
//
//    logger->debug("Writing request: " + formattedRequest);
//
//	ssize_t result = write(fd, formattedRequest.c_str(), formattedRequest.length());
//
//    ssize_t bytesWritten = write(pipes_[requestPipePath_], formattedRequest.c_str(), formattedRequest.length());
//	if (bytesWritten == -1) {
//		if (errno == EAGAIN) {
//			logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
//		} else {
//			logger->error("Failed to write to request pipe: " + requestPipePath_ + " - " + strerror(errno));
//		}
//		return false;
//	}
//
//    if (bytesWritten != static_cast<ssize_t>(formattedRequest.size())) {
//        logger->error("Failed to write the full request. Bytes written: " + std::to_string(bytesWritten));
//        return false;
//    }
//
//    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));
//
//    std::thread readerThread([this, callback]() {
//        this->readResponse(callback);
//    });
//    readerThread.detach();
//
//    return true;
//}
//
//auto IPCCore::readResponse(ResponseCallback callback) -> std::string {
//    logger->debug("IPCCore::readResponse() called");
//
//    int fd = pipes_[responsePipePath_];
//
//    if (fd == -1) {
//        logger->error("Response pipe is not open for reading.");
//        if (!openPipeForRead(responsePipePath_, true)) {  // Open in non-blocking mode
//            return "";
//        }
//        fd = pipes_[responsePipePath_];  // Reassign fd after reopening the pipe
//    }
//
//    const std::string startMarker = "START_";
//    const std::string endMarker = "END_OF_MESSAGE";
//    std::string requestId;
//
//    const size_t START_MARKER_SIZE = 6;
//    const size_t REQUEST_ID_SIZE = 8;
//    const size_t HEADER_SIZE = START_MARKER_SIZE + REQUEST_ID_SIZE + 8;
//    std::array<char, HEADER_SIZE + 1> header{}; // +1 for null termination
//    ssize_t bytesRead = 0;
//    size_t totalHeaderRead = 0;
//
//    // Retry loop in case of empty or partial reads
//    int retry_count = 0;
//    while (totalHeaderRead < HEADER_SIZE && retry_count < MAX_READ_RETRIES) {
//        if (stopIPC_) {
//            logger->info("IPC write initialization cancelled during read pipe setup.");
//            return "";
//        }
//        auto startIt = header.begin() + totalHeaderRead;
//        bytesRead = read(fd, &(*startIt), HEADER_SIZE - totalHeaderRead);
//
//        logger->debug("Header partial read: " + std::string(header.data(), totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));
//
//        if (bytesRead < 0) {
//            if (errno == EAGAIN || errno == EWOULDBLOCK) {
//                retry_count++;
//                usleep(DELAY_BETWEEN_READS);
//                continue;
//            } else {
//                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
//                return "";
//            }
//        }
//
//        if (bytesRead == 0) {
//            retry_count++;
//            usleep(DELAY_BETWEEN_READS);
//            continue;
//        }
//
//        totalHeaderRead += bytesRead;
//    }
//
//    if (totalHeaderRead != HEADER_SIZE) {
//        logger->error("Failed to read the full header after " + std::to_string(retry_count) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
//        return "";
//    }
//
//    logger->debug("Full header received: " + std::string(header.data(), totalHeaderRead));
//
//    // size_t instead of int because comparisons
//    size_t messageSize = 0;
//    try {
//        // Extract the response size (last 8 characters of the header)
//        std::string messageSizeStr(header.data() + 14);  // NOLINT Skip 'START_' and the 8 characters of request ID
//        messageSize = std::stoull(messageSizeStr);  // Convert to size_t
//    } catch (const std::invalid_argument& e) {
//        logger->error("Invalid header. Could not parse message size: " + std::string(e.what()));
//        return "";
//    } catch (const std::out_of_range& e) {
//        logger->error("Header size out of range: " + std::string(e.what()));
//        return "";
//    }
//
//    logger->debug("Message size to read: " + std::to_string(messageSize));
//
//    // init to empty string for callbacks expecting a string arg
//    std::string message = "";
//    size_t totalBytesRead = 0;
//    std::vector<char> buffer(BUFFER_SIZE);
//
//    while (totalBytesRead < messageSize + endMarker.size()) {
//        if (stopIPC_) {
//            logger->info("IPC write initialization cancelled during read pipe setup.");
//            return "";
//        }
//        size_t bytesToRead = std::min(BUFFER_SIZE, messageSize + endMarker.size() - totalBytesRead);
//        ssize_t bytesRead = read(fd, buffer.data(), bytesToRead);
//
//        if (bytesRead <= 0) {
//            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
//            usleep(DELAY_BETWEEN_READS);
//            continue;
//        }
//
//        message.append(buffer.data(), bytesRead);
//        totalBytesRead += bytesRead;
//        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));
//
//        // check for end marker in the accumulated message
//        if (message.size() >= endMarker.size()) {
//            if (message.compare(message.size() - endMarker.size(), endMarker.size(), endMarker) == 0) {
//                logger->debug("End of message marker found.");
//                message = message.substr(0, message.size() - endMarker.size()); // Remove the end marker
//                break;
//            }
//        }
//    }
//
//    logger->debug("Total bytes read: " + std::to_string(totalBytesRead - endMarker.size()));
//
//    logger->debug("Message read from response pipe: " + responsePipePath_);
//    if (message.length() > MESSAGE_TRUNCATE_CHARS) {
//        logger->debug("Message truncated to 100 characters");
//        logger->debug("Message: " + message.substr(0, MESSAGE_TRUNCATE_CHARS));
//    } else {
//        logger->debug("Message: " + message);
//    }
//
//    if (callback && !stopIPC_) {
//        callback(message);
//    }
//
//    return message;
//}
