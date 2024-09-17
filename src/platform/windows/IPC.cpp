#include <windows.h>
#include <functional>
#include <iostream>
#include <fstream>
#include <cstring>

#include "IPC.h"
#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PluginManager.h"

IPC::IPC(ApplicationManager& appManager)
    : app_(appManager), log_(appManager.getLogHandler()) {
    init();
}

IPC::~IPC() {
    for (auto& pipe : pipes_) {
        if (pipe.second != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe.second);
        }
    }
}

bool IPC::init() {
    log_->debug("IPC::init() called");

    if (!createPipe(requestPipePath) || !createPipe(responsePipePath)) {
        log_->debug("IPC::init() failed");
        return false;
    }

    // Make sure we can open/read the response pipe
    if (!openPipeForRead(responsePipePath)) {
        log_->debug("IPC::init() failed to open pipes for initial communication");
        return false;
    }

    // Open the request pipe for writing after some delay
    HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (hTimer) {
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -10000000LL; // 1-second delay

        if (SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
            WaitForSingleObject(hTimer, INFINITE);
            if (openPipeForWrite(requestPipePath)) {
                log_->info("Request pipe successfully opened for writing");
                writeRequest("READY");
            }
        }
        CloseHandle(hTimer);
    }

    log_->info("IPC::init() finished");
    return true;
}

void IPC::removePipeIfExists(const std::string& pipe_name) {
    // No equivalent of `unlink` for named pipes on Windows, so this is a no-op.
    log_->debug("No need to remove pipes on Windows: " + pipe_name);
}

bool IPC::createPipe(const std::string& pipe_name) {
    log_->debug("Creating named pipe: " + pipe_name);

    HANDLE pipe = CreateNamedPipeA(
        pipe_name.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096, 4096, 0, NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
        log_->error("Failed to create named pipe: " + pipe_name + " - " + std::to_string(GetLastError()));
        return false;
    }

    pipes_[pipe_name] = pipe;
    return true;
}

bool IPC::openPipeForWrite(const std::string& pipe_name) {
    HANDLE pipe = CreateFileA(
        pipe_name.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
        log_->error("Failed to open pipe for writing: " + pipe_name + " - " + std::to_string(GetLastError()));
        return false;
    }

    pipes_[pipe_name] = pipe;
    log_->debug("Pipe opened for writing: " + pipe_name);
    return true;
}

bool IPC::openPipeForRead(const std::string& pipe_name) {
    HANDLE pipe = CreateFileA(
        pipe_name.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
        log_->error("Failed to open pipe for reading: " + pipe_name + " - " + std::to_string(GetLastError()));
        return false;
    }

    pipes_[pipe_name] = pipe;
    log_->info("Pipe opened for reading: " + pipe_name);
    return true;
}

bool IPC::writeRequest(const std::string& message) {
    HANDLE pipe = pipes_[requestPipePath];
    if (pipe == INVALID_HANDLE_VALUE) {
        log_->error("Request pipe not opened for writing: " + requestPipePath);
        return false;
    }

    return writeToPipe(pipe, message);
}

void IPC::drainPipe(HANDLE pipe) {
    char buffer[4096];
    DWORD bytesRead;
    do {
        ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL);
    } while (bytesRead > 0);
}

std::string IPC::readResponse() {
    log_->debug("IPC::readResponse() called");

    HANDLE pipe = pipes_[responsePipePath];

    if (pipe == INVALID_HANDLE_VALUE) {
        log_->error("Response pipe is not open for reading.");
        if (!openPipeForRead(responsePipePath)) {  // Open the pipe in non-blocking mode
            return "";
        }
        pipe = pipes_[responsePipePath];  // Reassign pipe after reopening
    }

    const size_t HEADER_SIZE = 8;
    char header[HEADER_SIZE + 1];  // +1 for null-termination
    DWORD bytesRead = 0;
    size_t totalHeaderRead = 0;

    // Retry loop in case of empty reads
    int retry_count = 0;
    int max_retries = 5000;
    while (totalHeaderRead < HEADER_SIZE && retry_count < max_retries) {
        BOOL result = ReadFile(pipe, header + totalHeaderRead, HEADER_SIZE - totalHeaderRead, &bytesRead, NULL);
        if (!result || bytesRead == 0) {
            log_->error("Failed to read the full header. Bytes read: " + std::to_string(totalHeaderRead));
            retry_count++;
            Sleep(20);  // Sleep for 20ms
            continue;
        }
        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != HEADER_SIZE) {
        log_->error("Failed to read the full header after " + std::to_string(retry_count) + " retries.");
        return "";
    }

    header[HEADER_SIZE] = '\0';  // Null-terminate the header string

    // Convert the header to an integer representing the message size
    size_t messageSize = std::stoull(header);  // Handle larger numbers
    log_->debug("Message size to read: " + std::to_string(messageSize));

    // Step 2: Read the message in chunks
    std::string message;
    size_t totalBytesRead = 0;
    const size_t bufferSize = 4096;  // Read in chunks of 4096 bytes
    char buffer[bufferSize];

    while (totalBytesRead < messageSize) {
        size_t bytesToRead = std::min(bufferSize, messageSize - totalBytesRead);
        BOOL result = ReadFile(pipe, buffer, bytesToRead, &bytesRead, NULL);

        if (!result || bytesRead == 0) {
            log_->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            break;
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
        log_->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));
    }

    log_->debug("Total bytes read: " + std::to_string(totalBytesRead));

    // Verify if the message was fully read
    if (totalBytesRead != messageSize) {
        log_->error("Message read was incomplete.");
    } else {
        log_->debug("Message: read the entire message.");
    }

    log_->debug("Message read from response pipe: " + responsePipePath);
    if (message.length() > 100) {
        log_->debug("Message truncated to 100 characters");
        log_->debug("Message: " + message.substr(0, 100));
    } else {
        log_->debug("Message: " + message);
    }

    return message;
}

bool IPC::initReadWithEventLoop(std::function<void(const std::string&)> callback) {
    log_->debug("IPC::initReadWithEventLoop() called");

    HANDLE pipe = pipes_[responsePipePath];

    if (pipe == INVALID_HANDLE_VALUE) {
        log_->debug("IPC::initReadWithEventLoop() failed to open pipes for reading");
        if (!openPipeForRead(responsePipePath)) {
            return false;
        }
        pipe = pipes_[responsePipePath];
    }

    // Create a thread for reading data in an event loop
    CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
        auto* context = static_cast<std::pair<IPC*, std::function<void(const std::string&)>>*>(param);
        IPC* ipc = context->first;
        auto& callback = context->second;

        const int MAX_EMPTY_READS = 20;
        int emptyReadCounter = 0;

        while (true) {
            std::string response = ipc->readResponseInternal(ipc->pipes_[ipc->responsePipePath]);
            if (!response.empty()) {
                callback(response);
                emptyReadCounter = 0;
            } else {
                log_->debug("still transmitting");
                emptyReadCounter++;
                if (emptyReadCounter > MAX_EMPTY_READS) {
                    log_->error("Maximum empty reads reached. Terminating read attempt.");
                    emptyReadCounter = 0;
                    break;  // Exit the loop if too many empty reads
                }
            }

            Sleep(100);  // Poll every 100ms
        }

        delete context;
        return 0;
        }, new std::pair<IPC*, std::function<void(const std::string&)>>(this, callback), 0, NULL);

    return true;
}

std::string IPC::readResponseInternal(HANDLE pipe) {
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
    DWORD bytesRead = 0;

    // Step 1: Read the start marker and header, but only once it's fully received
    if (!headerRead) {
        while (headerBuffer.size() < HEADER_SIZE) {
            char headerChunk[HEADER_SIZE];
            size_t headerBytesToRead = HEADER_SIZE - headerBuffer.size();

            BOOL result = ReadFile(pipe, headerChunk, headerBytesToRead, &bytesRead, NULL);
            if (result && bytesRead > 0) {
                headerBuffer.append(headerChunk, bytesRead);
            } else if (!result || bytesRead == 0) {
                if (GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_MORE_DATA) {
                    log_->debug("Waiting for more data.");
                    return ""; // No data available right now, but the pipe is still open.
                } else {
                    log_->error("Failed to read the header. Error: " + std::to_string(GetLastError()));
                    return ""; // An unrecoverable error occurred, signal this as an empty response.
                }
            }
        }

        // Now we have the full header
        if (headerBuffer.compare(0, START_MARKER_SIZE, startMarker) != 0) {
            log_->error("Invalid start marker. Discarding data.");
            headerBuffer.clear(); // Clear the buffer and wait for a valid start marker
            return "";
        }

        requestId = headerBuffer.substr(START_MARKER_SIZE, REQUEST_ID_SIZE);
        messageSize = std::stoull(headerBuffer.substr(START_MARKER_SIZE + REQUEST_ID_SIZE, 8));
        log_->debug("Header read complete. Request ID: " + requestId + ", Expected message size: " + std::to_string(messageSize));

        headerRead = true; // Mark the header as read
    }

    // Step 2: Read the message content in chunks based on the size from the header
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t totalBytesRead = message.size(); // Track progress using the accumulated message size

    while (totalBytesRead < messageSize + endMarker.size()) {
        size_t bytesToRead = std::min(bufferSize, messageSize + endMarker.size() - totalBytesRead);
        BOOL result = ReadFile(pipe, buffer, bytesToRead, &bytesRead, NULL);

        if (bytesRead == 0) {
            log_->debug("End of file or no data available temporarily. Waiting for more data.");
            return "";  // No more data is available at this moment, but we should wait for more.
        } else if (!result) {
            if (GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_MORE_DATA) {
                log_->debug("Waiting for more data.");
                return ""; // No data available right now, but the pipe is still open.
            } else {
                log_->error("Failed to read the message. Error: " + std::to_string(GetLastError()));
                return ""; // An unrecoverable error occurred, signal this as an empty response.
            }
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;

        log_->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // Check if the end marker is present in the accumulated message
        if (message.size() >= endMarker.size()) {
            if (message.compare(message.size() - endMarker.size(), endMarker.size(), endMarker) == 0) {
                log_->debug("End of message marker found.");
                message = message.substr(0, message.size() - endMarker.size()); // Remove the end marker
                break;
            }
        }
    }

    // Verify that the message was fully read
    if (totalBytesRead >= messageSize) {
        log_->info("Message: read the entire message.");
        std::string completeMessage = message;

        // Reset static variables for the next read operation
        message.clear();
        headerBuffer.clear();
        messageSize = 0;
        headerRead = false;

        return completeMessage;
    } else {
        log_->warn("Message read is incomplete. Waiting for more data.");
        return ""; // Return empty string, indicating that we're still waiting for more data.
    }
}

bool IPC::writeToPipe(HANDLE pipe, const std::string& message) {
    DWORD bytesWritten;
    if (!WriteFile(pipe, message.c_str(), message.length(), &bytesWritten, NULL)) {
        log_->error("Failed to write to pipe: " + std::to_string(GetLastError()));
        return false;
    }

    log_->info("Message written to pipe: " + message);
    return true;
}

std::string IPC::readFromPipe(HANDLE pipe) {
    char buffer[4096];
    DWORD bytesRead;

    if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        log_->error("Failed to read from pipe: " + std::to_string(GetLastError()));
        return "";
    }

    buffer[bytesRead] = '\0';
    log_->debug("Message read from pipe: " + std::string(buffer));
    return std::string(buffer);
}

