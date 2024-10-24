#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "LogGlobal.h"

#include "IPCDefinitions.h"
#include "IPCPipe.h"
#include "PipeUtil.h"

using std::shared_ptr;

namespace fs = std::filesystem;

IPCPipe::IPCPipe(std::shared_ptr<PipeUtil> pipeUtil)
    : p_(std::move(pipeUtil))
    , pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeAccess_()
{}

auto IPCPipe::getPipeUtil() -> std::shared_ptr<PipeUtil> {
    return p_;
}

auto IPCPipe::create() -> bool {
    return p_->createPipe();
}

auto IPCPipe::openPipe() -> bool {
    return p_->openPipe();
}

auto IPCPipe::cleanUp() -> void {
    p_->closePipe();
    p_->deletePipe();
    p_->setHandle(ipc::NULL_PIPE_HANDLE);
}

auto IPCPipe::drainPipe() -> void {
    p_->drainPipe();
}

// loops openPipe
auto IPCPipe::openPipeLoop() -> bool {
    logger->debug("Setting up pipe. Path: " + pipePath_.string());
    for (int attempt = 0; attempt < ipc::MAX_PIPE_SETUP_ATTEMPTS; ++attempt) {
        if (stopIPC_) {
            logger->warn("IPCQueue initialization cancelled.");
            return false;
        }
        if (p_->openPipe()) {
            logger->info("Pipe successfully opened");
            return true;
        }
        logger->warn("Attempt to open response pipe for reading failed. Retrying...");
        std::this_thread::sleep_for(ipc::PIPE_SETUP_RETRY_DELAY);
    }
    logger->error("Max attempts reached for opening response pipe");
    return false;
}

auto IPCPipe::writeRequest(ipc::Request request) -> bool {
    logger->debug("Writing request: " + request.formatted());
	if (p_->getHandle() == ipc::INVALID_PIPE_HANDLE) {
		if (!p_->openPipe()) {
		    logger->error("Request pipe not opened for writing: " + p_->getPath().string());
			return false;
		}
	}

    p_->drainPipe();
    p_->writeToPipe(request);

    return true;
}

auto IPCPipe::logMessage(const std::string& message) -> void {
    logger->debug("Message read from response pipe: " + p_->getPath().string());
    if (message.length() > ipc::MESSAGE_TRUNCATE_CHARS) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, ipc::MESSAGE_TRUNCATE_CHARS));
    } else {
        logger->debug("Message: " + message);
    }
}

auto IPCPipe::readHeader() -> std::optional<ipc::Header> {
    size_t totalHeaderRead = 0;
    std::string requestId;

    std::array<char, ipc::HEADER_SIZE + 1> header{}; // +1 for null termination
    ssize_t bytesRead = 0;

    // Retry loop in case of empty or partial reads
    for (int retry_count = 0; totalHeaderRead < ipc::HEADER_SIZE && retry_count < ipc::MAX_READ_RETRIES; ++retry_count) {
        if (stopIPC_) {
            logger->info("Cannot read header: IPC halted.");
            //throw std::runtime_error("Cannot read header: IPC Halted.");
            return std::nullopt;
        }
        auto startIt = header.begin() + totalHeaderRead;
        bytesRead = p_->readFromPipe(&(*startIt), ipc::HEADER_SIZE - totalHeaderRead);

        logger->debug("Header partial read: " + std::string(header.data(), totalHeaderRead) + " | Bytes just read: " + std::to_string(bytesRead));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                std::this_thread::sleep_for(ipc::DELAY_BETWEEN_READS);
                continue;
            } else {
                logger->error("Failed to read the full header. Error: " + std::string(strerror(errno)));
                //throw std::runtime_error("Failed to read the full error. Error: " + std::string(strerror(errno)));
                return std::nullopt;
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
        logger->error("Failed to read the full header after " + std::to_string(ipc::MAX_READ_RETRIES) + " retries. Total header bytes read: " + std::to_string(totalHeaderRead));
        //throw std::runtime_error("Failed to read the full error. Error: " + std::string(strerror(errno)));
        return std::nullopt;
    }

    logger->debug("Full header received: " + std::string(header.data(), totalHeaderRead));

    try {
        std::string_view headerView(header.data(), ipc::HEADER_SIZE);
        uint64_t requestId = std::stoull(std::string(headerView.substr(ipc::START_MARKER_SIZE, ipc::REQUEST_ID_SIZE)));
        size_t messageSize = std::stoull(std::string(headerView.substr(ipc::START_MARKER_SIZE + ipc::REQUEST_ID_SIZE, ipc::REQUEST_ID_SIZE)));
        return ipc::Header{requestId, messageSize};
    } catch (const std::exception& e) {
        logger->error("Failed to parse header: " + std::string(e.what()));
        return std::nullopt;
    }
}

auto IPCPipe::readMessage(size_t messageSize) -> std::optional<std::string> {
    // size_t instead of int because comparisons
    std::string message;
    message.reserve(messageSize + ipc::END_MARKER.size());
    size_t totalBytesRead = 0;
    std::vector<char> buffer(ipc::BUFFER_SIZE);

    while (totalBytesRead < messageSize + ipc::END_MARKER.size()) {
        if (stopIPC_) {
            logger->info("IPCQueue write initialization cancelled during read pipe setup.");
            return std::nullopt;
        }

        size_t bytesToRead = std::min(ipc::BUFFER_SIZE, messageSize + ipc::END_MARKER.size() - totalBytesRead);
        ssize_t bytesRead = p_->readFromPipe(buffer.data(), bytesToRead);
        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        if (bytesRead <= 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            std::this_thread::sleep_for(ipc::DELAY_BETWEEN_READS);
            continue;
        }

        message.append(buffer.data(), bytesRead);
        totalBytesRead += bytesRead;

        //if (message.compare(message.size() - ipc::END_MARKER.size(), ipc::END_MARKER.size(), ipc::END_MARKER) == 0) {
        if (message.size() >= ipc::END_MARKER.size() &&
            std::string_view(message).substr(message.size() - ipc::END_MARKER.size()) == ipc::END_MARKER) {
            message.resize(message.size() - ipc::END_MARKER.size());
            logger->debug("Total bytes read: " + std::to_string(totalBytesRead - ipc::END_MARKER.size()));
            return message;
        }
    }

    logger->error("Failed to find end marker in message");
    return std::nullopt;
}

 auto IPCPipe::readResponse(ipc::ResponseCallback callback) -> ipc::Response {
     logger->debug("IPCResponsePipe::readResponse() called");

     if (!p_->ensurePipeOpen()) {
         return {ipc::ResponseType::Error, std::nullopt};
     }

     auto header = readHeader();
     if (!header) {
         return {ipc::ResponseType::Error, std::nullopt};
     }

     auto message = readMessage(header->messageSize);
     if (!message) {
         return {ipc::ResponseType::Error, std::nullopt};
     }

     logMessage(*message);

     if (callback && !stopIPC_) {
         (*callback)(*message);
     }

     return {ipc::ResponseType::Success, std::move(*message)};
 }

