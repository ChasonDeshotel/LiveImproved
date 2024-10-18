#define NOMINMAX
#include <windows.h>
#include <functional>
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdexcept>

#include "LogGlobal.h"

#include "IPCCore.h"
#include "IPluginManager.h"

IPCCore::IPCCore()
    : IIPCCore() {
    init();
}

IPCCore::~IPCCore() {
    for (auto& pipe : pipes_) {
        if (pipe.second != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe.second);
        }
    }
}

bool IPCCore::init() {
    logger->debug("IPCCore::init() called");

    // Create the write (outbound) pipe and read (inbound) pipe
    if (!createWritePipe(requestPipePath) || !createReadPipe(responsePipePath)) {
        logger->error("Failed to create pipes.");
        return false;
    }

    bool isReadInitialized = false;
    bool isWriteInitialized = false;

    // Wait for the client to connect to both pipes
    while (!isReadInitialized || !isWriteInitialized) {
        if (!isReadInitialized && connectToPipe(responsePipePath)) {
            logger->info("Response pipe connected (read).");
            isReadInitialized = true;
            logger->debug("read init");
        }

        if (!isWriteInitialized && connectToPipe(requestPipePath)) {
            logger->info("Request pipe connected (write).");
            isWriteInitialized = true;
            logger->debug("write init");
        }
        logger->debug("retry");
        Sleep(1000);  // Sleep between attempts
    }

    writeRequest("READY");
    logger->info("IPCCore initialized successfully.");
    return true;
}

bool IPCCore::createWritePipe(const std::string& pipe_name) {
    // Create write (outbound) pipe
    HANDLE pipe = CreateNamedPipeA(
        pipe_name.c_str(),
        PIPE_ACCESS_OUTBOUND,  // Server will write to this pipe
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Failed to create write pipe: " + pipe_name);
        return false;
    }
    pipes_[pipe_name] = pipe;
    return true;
}

bool IPCCore::createReadPipe(const std::string& pipe_name) {
    // Create read (inbound) pipe
    HANDLE pipe = CreateNamedPipeA(
        pipe_name.c_str(),
        PIPE_ACCESS_INBOUND,  // Server will read from this pipe
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Failed to create read pipe: " + pipe_name);
        return false;
    }
    pipes_[pipe_name] = pipe;
    return true;
}

bool IPCCore::connectToPipe(const std::string& pipe_name) {
    HANDLE pipe = pipes_[pipe_name];
    if (pipe != INVALID_HANDLE_VALUE) {
        if (ConnectNamedPipe(pipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            return true;  // Pipe is now connected
        } else {
            logger->error("Failed to connect to pipe: " + pipe_name);
        }
    }
    return false;
}

void IPCCore::removePipeIfExists(const std::string& pipe_name) {
    // No equivalent of `unlink` for named pipes on Windows, so this is a no-op.
    logger->debug("No need to remove pipes on Windows: " + pipe_name);
}

bool IPCCore::openPipeForWrite(const std::string& pipe_name) {
    int retry_count = 5;  // Number of retries
    HANDLE pipe;

    while (retry_count > 0) {
        pipe = CreateFileA(
            pipe_name.c_str(),   // Full pipe name
            GENERIC_WRITE,            // Write access only
            0,                        // No sharing
            NULL,                     // Default security attributes
            OPEN_EXISTING,            // Open existing pipe
            0,                        // Default attributes
            NULL                      // No template file
        );

        if (pipe != INVALID_HANDLE_VALUE) {
            break;  // Pipe opened successfully
        }

        DWORD error = GetLastError();
        if (error == ERROR_PIPE_BUSY) {
            // Pipe is busy, wait and retry
            if (WaitNamedPipeA(pipe_name.c_str(), 5000)) {
                // Pipe is available within timeout, retry opening
                retry_count--;
                continue;
            } else {
                logger->error("Write pipe is busy and timeout occurred: " + pipe_name);
                return false;
            }
        } else if (error == ERROR_ACCESS_DENIED) {
            logger->error("Access denied when opening write pipe: " + pipe_name + " - " + std::to_string(error));
            return false;
        } else {
            logger->error("Failed to open pipe for writing: " + pipe_name + " - " + std::to_string(error));
            return false;
        }
    }

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Failed to open pipe for writing after retries: " + pipe_name);
        return false;
    }

    pipes_[pipe_name] = pipe;
    logger->debug("Pipe opened for writing: " + pipe_name);
    return true;
}

bool IPCCore::openPipeForRead(const std::string& pipe_name) {
    HANDLE pipe;

	pipe = CreateFileA(
		pipe_name.c_str(),   // Full pipe name
		GENERIC_READ,            // Write access only
		0,                        // No sharing
		NULL,                     // Default security attributes
		OPEN_EXISTING,            // Open existing pipe
		0,                        // Default attributes
		NULL                      // No template file
	);

	if (pipe != INVALID_HANDLE_VALUE) {
		return false;  // Pipe opened successfully
	}

	DWORD error = GetLastError();
	if (error == ERROR_PIPE_BUSY) {
		// Pipe is busy, wait and retry
		if (WaitNamedPipeA(pipe_name.c_str(), 5000)) {
			// Pipe is available within timeout, retry opening
            return false;
		} else {
			logger->error("Read pipe is busy and timeout occurred: " + pipe_name);
			return false;
		}
	} else if (error == ERROR_ACCESS_DENIED) {
		logger->error("Access denied when opening read pipe: " + pipe_name + " - " + std::to_string(error));
		return false;
	} else {
		logger->error("Failed to open pipe for reading: " + pipe_name + " - " + std::to_string(error));
		return false;
	}

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Failed to open pipe for reading after retries: " + pipe_name);
        return false;
    }

    pipes_[pipe_name] = pipe;
    logger->debug("Pipe opened for reading: " + pipe_name);
    return true;
}
bool IPCCore::writeRequest(const std::string& message) {
    HANDLE pipe = pipes_[requestPipePath];
    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Request pipe not opened for writing: " + requestPipePath);
        return false;
    }

    return writeToPipe(pipe, message);
}

void IPCCore::drainPipe(HANDLE pipe) {
    char buffer[4096];
    DWORD bytesRead;
    do {
        ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL);
    } while (bytesRead > 0);
}

std::string IPCCore::readResponse() {
    logger->debug("IPCCore::readResponse() called");

    HANDLE pipe = pipes_[responsePipePath];

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->error("Response pipe is not open for reading.");
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
            logger->error("Failed to read the full header. Bytes read: " + std::to_string(totalHeaderRead));
            retry_count++;
            Sleep(20);  // Sleep for 20ms
            continue;
        }
        totalHeaderRead += bytesRead;
    }

    if (totalHeaderRead != HEADER_SIZE) {
        logger->error("Failed to read the full header after " + std::to_string(retry_count) + " retries.");
        return "";
    }

    header[HEADER_SIZE] = '\0';  // Null-terminate the header string

    // Convert the header to an integer representing the message size
    size_t messageSize = std::stoull(header);  // Handle larger numbers
    logger->debug("Message size to read: " + std::to_string(messageSize));

    // Step 2: Read the message in chunks
    std::string message;
    size_t totalBytesRead = 0;
    const size_t bufferSize = 4096;  // Read in chunks of 4096 bytes
    char buffer[bufferSize];

    while (totalBytesRead < messageSize) {
        size_t bytesToRead = std::min(bufferSize, messageSize - totalBytesRead);
        BOOL result = ReadFile(pipe, buffer, bytesToRead, &bytesRead, NULL);

        if (!result || bytesRead == 0) {
            logger->error("Failed to read the message or end of file reached. Total bytes read: " + std::to_string(totalBytesRead));
            break;
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));
    }

    logger->debug("Total bytes read: " + std::to_string(totalBytesRead));

    // Verify if the message was fully read
    if (totalBytesRead != messageSize) {
        logger->error("Message read was incomplete.");
    } else {
        logger->debug("Message: read the entire message.");
    }

    logger->debug("Message read from response pipe: " + responsePipePath);
    if (message.length() > 100) {
        logger->debug("Message truncated to 100 characters");
        logger->debug("Message: " + message.substr(0, 100));
    } else {
        logger->debug("Message: " + message);
    }

    return message;
}

bool IPCCore::initReadWithEventLoop(std::function<void(const std::string&)> callback) {
    logger->debug("IPCCore::initReadWithEventLoop() called");

    HANDLE pipe = pipes_[responsePipePath];

    if (pipe == INVALID_HANDLE_VALUE) {
        logger->debug("IPCCore::initReadWithEventLoop() failed to open pipes for reading");
        if (!openPipeForRead(responsePipePath)) {
            return false;
        }
        pipe = pipes_[responsePipePath];
    }

    // Create a thread for reading data in an event loop
    CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
        auto* context = static_cast<std::pair<IPCCore*, std::function<void(const std::string&)>>*>(param);
        IPCCore* ipc = context->first;
        auto& callback = context->second;

        const int MAX_EMPTY_READS = 20;
        int emptyReadCounter = 0;

        while (true) {
            std::string response = ipc->readResponseInternal(ipc->pipes_[ipc->responsePipePath]);
            if (!response.empty()) {
                callback(response);
                emptyReadCounter = 0;
            } else {
                logger->debug("still transmitting");
                emptyReadCounter++;
                if (emptyReadCounter > MAX_EMPTY_READS) {
                    logger->error("Maximum empty reads reached. Terminating read attempt.");
                    emptyReadCounter = 0;
                    break;  // Exit the loop if too many empty reads
                }
            }

            Sleep(100);  // Poll every 100ms
        }

        delete context;
        return 0;
        }, new std::pair<IPCCore*, std::function<void(const std::string&)>>(this, callback), 0, NULL);

    return true;
}

std::string IPCCore::readResponseInternal(HANDLE pipe) {
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
                    logger->debug("Waiting for more data.");
                    return ""; // No data available right now, but the pipe is still open.
                } else {
                    logger->error("Failed to read the header. Error: " + std::to_string(GetLastError()));
                    return ""; // An unrecoverable error occurred, signal this as an empty response.
                }
            }
        }

        // Now we have the full header
        if (headerBuffer.compare(0, START_MARKER_SIZE, startMarker) != 0) {
            logger->error("Invalid start marker. Discarding data.");
            headerBuffer.clear(); // Clear the buffer and wait for a valid start marker
            return "";
        }

        requestId = headerBuffer.substr(START_MARKER_SIZE, REQUEST_ID_SIZE);
        messageSize = std::stoull(headerBuffer.substr(START_MARKER_SIZE + REQUEST_ID_SIZE, 8));
        logger->debug("Header read complete. Request ID: " + requestId + ", Expected message size: " + std::to_string(messageSize));

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
            logger->debug("End of file or no data available temporarily. Waiting for more data.");
            return "";  // No more data is available at this moment, but we should wait for more.
        } else if (!result) {
            if (GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_MORE_DATA) {
                logger->debug("Waiting for more data.");
                return ""; // No data available right now, but the pipe is still open.
            } else {
                logger->error("Failed to read the message. Error: " + std::to_string(GetLastError()));
                return ""; // An unrecoverable error occurred, signal this as an empty response.
            }
        }

        message.append(buffer, bytesRead);
        totalBytesRead += bytesRead;

        logger->debug("Chunk read: " + std::to_string(bytesRead) + " bytes. Total bytes read: " + std::to_string(totalBytesRead));

        // Check if the end marker is present in the accumulated message
        if (message.size() >= endMarker.size()) {
            if (message.compare(message.size() - endMarker.size(), endMarker.size(), endMarker) == 0) {
                logger->debug("End of message marker found.");
                message = message.substr(0, message.size() - endMarker.size()); // Remove the end marker
                break;
            }
        }
    }

    // Verify that the message was fully read
    if (totalBytesRead >= messageSize) {
        logger->info("Message: read the entire message.");
        std::string completeMessage = message;

        // Reset static variables for the next read operation
        message.clear();
        headerBuffer.clear();
        messageSize = 0;
        headerRead = false;

        return completeMessage;
    } else {
        logger->warn("Message read is incomplete. Waiting for more data.");
        return ""; // Return empty string, indicating that we're still waiting for more data.
    }
}

bool IPCCore::writeToPipe(HANDLE pipe, const std::string& message) {
    DWORD bytesWritten;
    if (!WriteFile(pipe, message.c_str(), message.length(), &bytesWritten, NULL)) {
        logger->error("Failed to write to pipe: " + std::to_string(GetLastError()));
        return false;
    }

    logger->info("Message written to pipe: " + message);
    return true;
}

std::string IPCCore::readFromPipe(HANDLE pipe) {
    char buffer[4096];
    DWORD bytesRead;

    if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        logger->error("Failed to read from pipe: " + std::to_string(GetLastError()));
        return "";
    }

    buffer[bytesRead] = '\0';
    logger->debug("Message read from pipe: " + std::string(buffer));
    return std::string(buffer);
}


