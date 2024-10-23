#include <cerrno>
#include <thread>
#include <fcntl.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "LogGlobal.h"
#include "PathManager.h"

#include "IPCPipe.h"
#include "IPCDefinitions.h"
#include "IPCResponsePipe.h"

using ipc::Response;

IPCResponsePipe::IPCResponsePipe()
    : IPCPipe(), pipeHandle_(ipc::INVALID_PIPE_HANDLE),
      pipePath_(PathManager().responsePipe()),
      pipeFlags_(O_RDONLY | O_NONBLOCK) {
  // set on base class
  setPipePath(pipePath_);
  setPipeFlags(pipeFlags_);
}

IPCResponsePipe::~IPCResponsePipe() = default;

auto IPCResponsePipe::readResponse(ipc::ResponseCallback callback) -> ipc::Response {
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

auto IPCResponsePipe::writeToPipe(ipc::Request) -> bool {
    logger->error("unimplemented in response pipe");
    return false;
}

auto IPCResponsePipe::logMessage(const std::string& message) -> void {

    logger->debug("Message read from response pipe: " + this->string());
    if (message.length() > ipc::MESSAGE_TRUNCATE_CHARS) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, ipc::MESSAGE_TRUNCATE_CHARS));
    } else {
        logger->debug("Message: " + message);
    }

}

//auto IPCRequestPipeBase::resetResponsePipe() -> void {
//    logger->debug("Resetting response pipe");
//    close(_responsePipeHandle_);
//    _responsePipeHandle_ = INVALID_PIPE_HANDLE;
//    if (!IPCRequestPipe::openResponsePipe(_responsePipePath_, _responsePipeHandle_)) {
//        logger->error("Failed to reopen response pipe");
//    } else {
//        logger->info("Response pipe reopened successfully");
//    }
//}
//auto IPCRequestPipeBase::removePipeIfExists(const std::string& pipeName) -> void {
//    // TODO: use filesystem remove
//    if (access(pipeName.c_str(), F_OK) != -1) {
//        // File exists, so remove it
//        if (unlink(pipeName.c_str()) == 0) {
//            logger->debug("Removed existing pipe: " + pipeName);
//        } else {
//            logger->error("Failed to remove existing pipe: " + pipeName + " - " + strerror(errno));
//        }
//    }
//}
