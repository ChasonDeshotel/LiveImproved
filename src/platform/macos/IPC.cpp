#include "IPC.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <dispatch/dispatch.h>

#include "IPC.h"
#include "LogHandler.h"

#include "PluginManager.h"

IPC::IPC(std::function<std::shared_ptr<ILogHandler>()> logHandler)
        : logHandler_(std::move(logHandler))
    {
        if (!logHandler_()) {
            throw std::invalid_argument("IPC requires valid logHandler and pluginManager");
        }
    }

IPC::~IPC() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }
}

bool IPC::init() {
    log_()->debug("IPC::init() called");

    if (!createPipe(requestPipePath) || !createPipe(responsePipePath)) {
        log_()->debug("IPC::init() failed");
        return false;
    }

    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    __block bool readSuccess = false;
    __block bool writeSuccess = false;
    __block int readAttempts = 0;
    __block int writeAttempts = 0;
    __block int maxAttempts = 100;

    // Read pipe initialization
    dispatch_group_async(group, queue, ^{
        dispatch_source_t readTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        dispatch_source_set_timer(readTimer, DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC, 0);
        dispatch_source_set_event_handler(readTimer, ^{
            if (openPipeForRead(responsePipePath, true)) {
                log_()->info("Response pipe successfully opened for reading");
                readSuccess = true;
                dispatch_source_cancel(readTimer);
            } else {
                log_()->error("Attempt to open response pipe for reading failed. Retrying...");
                if (++readAttempts >= maxAttempts) {
                    log_()->error("Max attempts reached for opening response pipe");
                    dispatch_source_cancel(readTimer);
                }
            }
        });
        dispatch_resume(readTimer);
    });

    // Write pipe initialization
    dispatch_group_async(group, queue, ^{
        dispatch_source_t writeTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        dispatch_source_set_timer(writeTimer, DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC, 0);
        dispatch_source_set_event_handler(writeTimer, ^{
            if (openPipeForWrite(requestPipePath, true)) {
                log_()->info("Request pipe successfully opened for writing");
                writeSuccess = true;
                dispatch_source_cancel(writeTimer);
            } else {
                log_()->error("Attempt to open request pipe for writing failed. Retrying...");
                if (++writeAttempts >= maxAttempts) {
                    log_()->error("Max attempts reached for opening request pipe");
                    dispatch_source_cancel(writeTimer);
                }
            }
        });
        dispatch_resume(writeTimer);
    });

    // Wait for both operations to complete
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

    // for async/callback
    //dispatch_group_notify(group, queue, ^{
    //    bool success = readSuccess && writeSuccess;
    //    if (success) {
    //        log_()->info("IPC::initAsync() finished successfully");
    //    } else {
    //        log_()->error("IPC::initAsync() failed");
    //    }
    //    // Call the callback on the main queue
    //    dispatch_async(dispatch_get_main_queue(), ^{
    //        callback(success);
    //    });
    //});

    if (readSuccess && writeSuccess) {
        log_()->info("IPC::init() finished successfully");
        return true;
    } else {
        log_()->error("IPC::init() failed");
        return false;
    }
}




// TODO *should*. idk anymore
// blocks/retries until IPC is available
//bool IPC::init() {
//    log_()->debug("IPC::init() called");
//
//    if (!createPipe(requestPipePath) || !createPipe(responsePipePath)) {
//        log_()->debug("IPC::init() failed");
//        return false;
//    }
//
//    // make sure we can open/read the response pipe
//    if (!openPipeForRead(responsePipePath, true)) {
//        log_()->debug("IPC::init() failed to open pipes for initial communication");
//        return false;
//    }
//    if (openPipeForWrite(requestPipePath, true)) {
//        log_()->info("request pipe successfully opened for writing");
//        return false;
//    }
//
//    dispatch_queue_t queue = dispatch_get_main_queue();
//    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
//    dispatch_source_set_timer(timer, DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC, 0);  // 100ms interval
//    dispatch_source_set_event_handler(timer, ^{
//        if (openPipeForWrite(requestPipePath, true)) {
//            log_()->info("request pipe successfully opened for writing");
//
//            dispatch_source_cancel(timer);
//        } else {
//            log_()->error("Attempt to open request pipe for writing failed. Retrying...");
//        }
//    });
//    dispatch_resume(timer);
//
//    log_()->info("IPC::init() finished");
//    return true;
//}

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

bool IPC::writeToPipe(const std::string& pipe_name, const std::string& message) {
    return false;
}

std::string IPC::readFromPipe(const std::string& pipe_name) {
    return std::string();
}

bool IPC::writeRequest(const std::string& message) {
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

    ssize_t result = write(fd, message.c_str(), message.length());
    if (result == -1) {
        if (errno == EAGAIN) {
            log_()->error("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            log_()->error("Failed to write to request pipe: " + requestPipePath + " - " + strerror(errno));
        }
        return false;
    }

    log_()->info("Message: (" + message + ") written to request pipe");
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

std::string IPC::readResponse() {
    log_()->debug("IPC::readResponse() called");

    int fd = pipes_[responsePipePath];

    if (fd == -1) {
        log_()->error("Response pipe is not open for reading.");
        if (!openPipeForRead(responsePipePath, true)) {  // Open in non-blocking mode
            return "";
        }
        fd = pipes_[responsePipePath];  // Reassign fd after reopening the pipe
    }

    const size_t HEADER_SIZE = 8;
    char header[HEADER_SIZE + 1];  // +1 for null-termination
    ssize_t bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty reads
    int retry_count = 0;
    int max_retries = 5000;
    while (totalHeaderRead < HEADER_SIZE && retry_count < max_retries) {
        bytesRead = read(fd, header + totalHeaderRead, HEADER_SIZE - totalHeaderRead);
        if (bytesRead <= 0) {
            log_()->error("Failed to read the full header. Bytes read: " + std::to_string(totalHeaderRead));
            retry_count++;
            usleep(20000);
            continue;
        }
        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != HEADER_SIZE) {
        log_()->error("Failed to read the full header after " + std::to_string(retry_count) + " retries.");
        return "";
    }

    header[HEADER_SIZE] = '\0';  // Null-terminate the header string

    // Convert the header to an integer representing the message size
    size_t messageSize = std::stoull(header);  // Handle larger numbers
    log_()->debug("Message size to read: " + std::to_string(messageSize));

    // Step 2: Read the message in chunks
    std::string message;
    size_t totalBytesRead = 0;
    const size_t bufferSize = 4096;  // Read in chunks of 4096 bytes
    char buffer[bufferSize];

    while (totalBytesRead < messageSize) {
        size_t bytesToRead = std::min(bufferSize, messageSize - totalBytesRead);
        bytesRead = read(fd, buffer, bytesToRead);

        if (bytesRead <= 0) {
            log_()->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            break;
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
        log_()->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));
    }

    log_()->debug("Total bytes read: " + std::to_string(totalBytesRead));

    // Verify if the message was fully read
    if (totalBytesRead != messageSize) {
        log_()->error("Message read was incomplete.");
    } else {
        log_()->debug("Message: read the entire message.");
    }

    log_()->debug("Message read from response pipe: " + responsePipePath);
    if (message.length() > 100) {
        log_()->debug("Message truncated to 100 characters");
        log_()->debug("Message: " + message.substr(0,100));
    } else {
        log_()->debug("Message: " + message);
    }
    return message;
}

bool IPC::initReadWithEventLoop(std::function<void(const std::string&)> callback) {
    log_()->debug("IPC::initReadWithEventLoop() called");

    int fd = pipes_[responsePipePath];

    if (fd == -1) {
        log_()->debug("IPC::initReadWithEventLoop() failed to open pipes for reading");
        if (!openPipeForRead(responsePipePath, true)) {  // Open in non-blocking mode
            return false;
        }
        fd = pipes_[responsePipePath];  // Reassign fd after reopening the pipe
    }

    // Set up a dispatch source to monitor the pipe for read availability
    dispatch_queue_t queue = dispatch_get_main_queue();
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd, 0, queue);

    if (!source) {
        log_()->error("Failed to create dispatch source.");
        return false;
    }

    const int MAX_EMPTY_READS = 20;
    __block int emptyReadCounter = 0;

    // Convert the C++ lambda to a dispatch_block_t
    dispatch_block_t block = ^{
        size_t estimated = dispatch_source_get_data(source);
        log_()->debug("Data available in pipe: " + std::to_string(estimated) + " bytes");

        if (estimated > 0) {
            std::string response = this->readResponseInternal(fd);
            if (!response.empty()) {
                callback(response);
                emptyReadCounter = 0;
            } else {
                log_()->debug("still transmitting");
                emptyReadCounter++;
                if (emptyReadCounter > MAX_EMPTY_READS) {
                    log_()->error("Maximum empty reads reached. Terminating read attempt.");
                    emptyReadCounter = 0;
                }
            }
        } else {
            log_()->warn("No data available to read.");
        }
    };

    // Set the event handler using the converted block
    dispatch_source_set_event_handler(source, block);

    dispatch_source_set_cancel_handler(source, ^{
        log_()->warn("Dispatch source canceled.");
        close(fd);
    });

    dispatch_resume(source);
    return true;
}

std::string IPC::readResponseInternal(int fd) {
    static size_t messageSize = 0;  // Static to retain value across calls
    static std::string message;     // Static to accumulate the message
    static std::string headerBuffer; // Static to accumulate header bytes
    static bool headerRead = false; // Static flag to ensure header is only read once
    static const std::string startMarker = "START_"; // Start of message marker
    static const std::string endMarker = "END_OF_MESSAGE"; // End of message marker
    static std::string requestId; // Store request ID

    const size_t START_MARKER_SIZE = 6; // "START_" size
    const size_t REQUEST_ID_SIZE = 8; // Request ID size
    const size_t HEADER_SIZE = START_MARKER_SIZE + REQUEST_ID_SIZE + 8; // Full header size
    ssize_t bytesRead = 0;

    // Step 1: Read the start marker and header, but only once it's fully received
    if (!headerRead) {
        while (headerBuffer.size() < HEADER_SIZE) {
            char headerChunk[HEADER_SIZE];
            size_t headerBytesToRead = HEADER_SIZE - headerBuffer.size();

            bytesRead = read(fd, headerChunk, headerBytesToRead);
            if (bytesRead > 0) {
                headerBuffer.append(headerChunk, bytesRead);
            } else if (bytesRead < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    log_()->debug("Waiting for more data.");
                    return ""; // No data available right now, but the pipe is still open.
                } else {
                    log_()->error("Failed to read the header. Error: " + std::string(strerror(errno)));
                    return ""; // An unrecoverable error occurred, signal this as an empty response.
                }
            } else {
                log_()->error("Unexpected end of file while reading the header.");
                return ""; // No data available and the pipe might be closed.
            }
        }

        // Now we have the full header
        if (headerBuffer.compare(0, START_MARKER_SIZE, startMarker) != 0) {
            log_()->error("Invalid start marker. Discarding data.");
            headerBuffer.clear(); // Clear the buffer and wait for a valid start marker
            return "";
        }

        requestId = headerBuffer.substr(START_MARKER_SIZE, REQUEST_ID_SIZE);
        messageSize = std::stoull(headerBuffer.substr(START_MARKER_SIZE + REQUEST_ID_SIZE, 8));
        log_()->debug("Header read complete. Request ID: " + requestId + ", Expected message size: " + std::to_string(messageSize));

        headerRead = true; // Mark the header as read
    }

    // Step 2: Read the message content in chunks based on the size from the header
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t totalBytesRead = message.size(); // Track progress using the accumulated message size

    while (totalBytesRead < messageSize + endMarker.size()) {
        size_t bytesToRead = std::min(bufferSize, messageSize + endMarker.size() - totalBytesRead);
        bytesRead = read(fd, buffer, bytesToRead);

        if (bytesRead == 0) {
            log_()->debug("End of file or no data available temporarily. Waiting for more data.");
            return "";  // No more data is available at this moment, but we should wait for more.
        } else if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                log_()->debug("Waiting for more data.");
                return ""; // No data available right now, but the pipe is still open.
            } else {
                log_()->error("Failed to read the message. Error: " + std::string(strerror(errno)));
                return ""; // An unrecoverable error occurred, signal this as an empty response.
            }
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

    // Verify that the message was fully read
    if (totalBytesRead >= messageSize) {
        log_()->info("Message: read the entire message.");
        std::string completeMessage = message;

        // Reset static variables for the next read operation
        message.clear();
        headerBuffer.clear();
        messageSize = 0;
        headerRead = false;

        return completeMessage;
    } else {
        log_()->warn("Message read is incomplete. Waiting for more data.");
        return ""; // Return empty string, indicating that we're still waiting for more data.
    }
}
