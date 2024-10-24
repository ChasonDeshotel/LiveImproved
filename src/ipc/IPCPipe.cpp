#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "LogGlobal.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"
#include "PipeUtil.h"

namespace fs = std::filesystem;

IPCPipe::IPCPipe(std::function<std::shared_ptr<PipeUtil>()> pipeUtil)
    : pipeUtil_(std::move(pipeUtil))
    , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeFlags_()
{}

auto IPCPipe::create() -> bool {
    auto p = pipeUtil_();
    return p->create();
}

auto IPCPipe::openPipe() -> bool {
    pipeHandle_ = open(pipePath_.c_str(), getPipeFlags()); // NOLINT
    if (pipeHandle_ == ipc::INVALID_PIPE_HANDLE) {
        logger->error("Failed to open pipe: " + pipePath_.string() + " - " + strerror(errno));
        return false;
    }

    setHandle(pipeHandle_);
    logger->debug("Pipe opened: " + pipePath_.string());
    logger->debug("Pipe opened handle: " + std::to_string(pipeHandle_));
    return true;
}

auto IPCPipe::cleanUp() -> void {
    if (pipeHandle_ != ipc::NULL_PIPE_HANDLE && pipeHandle_ != ipc::INVALID_PIPE_HANDLE) {
        close(pipeHandle_);
        logger->debug("closed pipe");
    }
    setHandleNull();

    if (! (fs::exists(pipePath_)) ) {
        logger->warn("pipe does not exist or is not a regular file: " + pipePath_.string());
        return;
    }
    if (std::filesystem::remove(pipePath_)) {
        logger->debug("deleted pipe: " + pipePath_.string());
    } else {
        logger->warn("Failed to remove request pipe file: " + pipePath_.string());
    }

    setHandleNull();
}

auto IPCPipe::drainPipe(size_t bufferSize) -> void {
    std::vector<char> buffer(bufferSize);
    ssize_t bytesRead = 0;
    do { // NOLINT - don't be stupid. Know and love the do-while
        bytesRead = read(pipeHandle_, const_cast<char*>(buffer.data()), buffer.size());
    } while (bytesRead > 0);
}

// loops openPipe
auto IPCPipe::openPipeLoop() -> bool {
    logger->debug("Setting up pipe. Path: " + pipePath_.string());
    for (int attempt = 0; attempt < ipc::MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue initialization cancelled.");
            return false;
        }
        if (openPipe()) {
            logger->info("Pipe successfully opened");

            // TODO
            // if response pipe
            // setHandleNull()

            return true;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(ipc::PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
    return false;
}

//auto IPCPipe::readResponse(ipc::ResponseCallback callback) -> ipc::Response {
//    logger->error("not implemented. Must be called from IPCResponsePipe.");
//    return {ipc::ResponseType::Error, std::nullopt};
//}

auto IPCPipe::writeRequest(ipc::Request request) -> bool {
	if (this->getHandle() == ipc::INVALID_PIPE_HANDLE) {
		if (!this->openPipe()) {
		    logger->error("Request pipe not opened for writing: " + this->string());
			return false;
		}
	}

    this->drainPipe(ipc::BUFFER_SIZE);

    logger->debug("Writing request: " + request.formatted());

    ssize_t bytesWritten = write(this->getHandle(), request.formatted().c_str(), request.formatted().length());
    if (bytesWritten == -1) {
        if (errno == EAGAIN) {
            logger->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            logger->error("Failed to write to request pipe: " + this->string() + " - " + strerror(errno));
        }
        return false;
    } else if (bytesWritten != request.formatted().length()) {
        logger->error("Incomplete write to request pipe. Wrote " + std::to_string(bytesWritten) + " of " + std::to_string(request.formatted().length()) + " bytes");
        return false;
    }
    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));

    return true;
}

auto IPCPipe::logMessage(const std::string& message) -> void {
    logger->debug("Message read from response pipe: " + this->string());
    if (message.length() > ipc::MESSAGE_TRUNCATE_CHARS) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, ipc::MESSAGE_TRUNCATE_CHARS));
    } else {
        logger->debug("Message: " + message);
    }
}

auto IPCPipe::readResponse(ipc::ResponseCallback callback) -> ipc::Response {
    logger->debug("IPCResponsePipe::readResponse() called");

    int fd = this->getHandle();
    logger->debug("read response handle: " + std::to_string(fd));

    if (fd == -1) {
        logger->error("Response pipe is not open for reading.");
        if (!this->openPipe()) {  // Open in non-blocking mode
          return {ipc::ResponseType::Error, std::nullopt};
        }
    }

    std::string requestId;

    std::array<char, ipc::HEADER_SIZE + 1> header{}; // +1 for null termination
    ssize_t bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty or partial reads
    int retry_count = 0;
    while (totalHeaderRead < ipc::HEADER_SIZE && retry_count < ipc::MAX_READ_RETRIES) {
        if (stopIPC_) {
            logger->info("IPCQueue write initialization cancelled during read pipe setup.");
            return {ipc::ResponseType::Error, std::nullopt};
        }
        auto startIt = header.begin() + totalHeaderRead;
        bytesRead = read(this->getHandle(), &(*startIt), ipc::HEADER_SIZE - totalHeaderRead);

        logger->debug("Header partial read: " + std::string(header.data(), totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                std::this_thread::sleep_for(ipc::DELAY_BETWEEN_READS);
                continue;
            } else {
                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
                return {ipc::ResponseType::Error, std::nullopt};
            }
        }

        if (bytesRead == 0) {
            retry_count++;
            std::this_thread::sleep_for(ipc::DELAY_BETWEEN_READS);
            continue;
        }

        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != ipc::HEADER_SIZE) {
        logger->error("Failed to read the full header after " + std::to_string(retry_count) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
        return {ipc::ResponseType::Error, std::nullopt};
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
        return {ipc::ResponseType::Error, std::nullopt};
    } catch (const std::out_of_range& e) {
        logger->error("Header size out of range: " + std::string(e.what()));
        return {ipc::ResponseType::Error, std::nullopt};
    }

    logger->debug("Message size to read: " + std::to_string(messageSize));

    // init to empty string for callbacks expecting a string arg
    std::string message = "";
    size_t totalBytesRead = 0;
    std::vector<char> buffer(ipc::BUFFER_SIZE);

    while (totalBytesRead < messageSize + ipc::END_MARKER.size()) {
        if (stopIPC_) {
            logger->info("IPCQueue write initialization cancelled during read pipe setup.");
            return {ipc::ResponseType::Error, std::nullopt};
        }
        size_t bytesToRead = std::min(ipc::BUFFER_SIZE, messageSize + ipc::END_MARKER.size() - totalBytesRead);
        ssize_t bytesRead = read(this->getHandle(), buffer.data(), bytesToRead);

        if (bytesRead <= 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            std::this_thread::sleep_for(ipc::DELAY_BETWEEN_READS);
            continue;
        }

        message.append(buffer.data(), bytesRead);
        totalBytesRead += bytesRead;
        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // check for end marker in the accumulated message
        if (message.size() >= ipc::END_MARKER.size()) {
            if (message.compare(message.size() - ipc::END_MARKER.size(), ipc::END_MARKER.size(), ipc::END_MARKER) == 0) {
                logger->debug("End of message marker found.");
                message = message.substr(0, message.size() - ipc::END_MARKER.size()); // Remove the end marker
                break;
            }
        }
    }

    logger->debug("Total bytes read: " + std::to_string(totalBytesRead - ipc::END_MARKER.size()));
    logMessage(message);

    if (callback.has_value() && !stopIPC_) {
        (*callback)(message);
    }

    return {ipc::ResponseType::Success, message};
}
