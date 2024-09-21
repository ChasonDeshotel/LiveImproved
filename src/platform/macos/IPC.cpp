#include "IPC.h"
#include <chrono>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <dispatch/dispatch.h>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <iomanip>
#include <sstream>

#include "IPC.h"
#include "LogHandler.h"

#include "PluginManager.h"

IPC::IPC(std::function<std::shared_ptr<ILogHandler>()> logHandler)
        : logHandler_(std::move(logHandler))
    {
        if (!logHandler_()) {
            throw std::invalid_argument("IPC requires valid logHandler and pluginManager");
        }
		requestWriterThread_ = std::thread(&IPC::requestWriter, this);
		responseReaderThread_ = std::thread(&IPC::responseReader, this);
    }

IPC::~IPC() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }
	shouldStop_ = true;
	cv_.notify_one();
	if (requestWriterThread_.joinable()) {
		requestWriterThread_.join();
	}
}

bool IPC::init() {
    log_()->debug("IPC::init() called");

    if (!createPipe(requestPipePath) || !createPipe(responsePipePath)) {
        log_()->debug("IPC::init() failed");
        return false;
    }

    dispatch_semaphore_t readSemaphore = dispatch_semaphore_create(0);
    dispatch_semaphore_t writeSemaphore = dispatch_semaphore_create(0);

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    __block bool readSuccess = false;
    __block bool writeSuccess = false;
    __block int readAttempts = 0;
    __block int writeAttempts = 0;
    __block int maxAttempts = 100;

	dispatch_async(queue, ^{
		dispatch_source_t readTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
		dispatch_source_set_timer(readTimer, DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC, 0);
		dispatch_source_set_event_handler(readTimer, ^{
			if (openPipeForRead(responsePipePath, true)) {
				log_()->info("Response pipe successfully opened for reading");
				readSuccess = true;
				dispatch_source_cancel(readTimer);
				dispatch_semaphore_signal(readSemaphore);
			} else {
				log_()->error("Attempt to open response pipe for reading failed. Retrying...");
				if (++readAttempts >= maxAttempts) {
					log_()->error("Max attempts reached for opening response pipe");
					dispatch_source_cancel(readTimer);
					dispatch_semaphore_signal(readSemaphore);
				}
			}
		});
		dispatch_resume(readTimer);
	});

	// Write pipe initialization
	dispatch_async(queue, ^{
		dispatch_source_t writeTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
		dispatch_source_set_timer(writeTimer, DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC, 0);
		dispatch_source_set_event_handler(writeTimer, ^{
			if (openPipeForWrite(requestPipePath, true)) {
				log_()->info("Request pipe successfully opened for writing");
				writeSuccess = true;
				dispatch_source_cancel(writeTimer);
				dispatch_semaphore_signal(writeSemaphore);
			} else {
				log_()->error("Attempt to open request pipe for writing failed. Retrying...");
				if (++writeAttempts >= maxAttempts) {
					log_()->error("Max attempts reached for opening request pipe");
					dispatch_source_cancel(writeTimer);
					dispatch_semaphore_signal(writeSemaphore);
				}
			}
		});
		dispatch_resume(writeTimer);
	});

	dispatch_semaphore_wait(readSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_semaphore_wait(writeSemaphore, DISPATCH_TIME_FOREVER);

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
        log_()->info("IPC::init() successfully opened both pipes");
        return true;
    } else {
        log_()->error("IPC::init() failed");
        return false;
    }
}

void IPC::trackRequest(uint64_t id, const std::string& status) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto& info = requestTracker_[id];
	if (status == "created") {
		info.creationTime = std::chrono::system_clock::now();
		info.status = "created";
	} else if (status == "written") {
		info.writeTime = std::chrono::system_clock::now();
		info.status = "written";
	} else if (status == "responded") {
		info.responseTime = std::chrono::system_clock::now();
		info.status = "completed";
		log_()->debug("Request " + std::to_string(id) + " completed in " +
					std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
						info.responseTime - info.creationTime).count()) + "ms");
		requestTracker_.erase(id);
	}
}

// TODO 
//void IPC::checkStuckRequests() {
//    std::lock_guard<std::mutex> lock(trackerMutex_);
//    auto now = std::chrono::system_clock::now();
//    for (const auto& pair : requestTracker_) {
//        auto& info = pair.second;
//        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - info.creationTime).count();
//        if (duration > 60) {  // Example: Flag requests older than 60 seconds
//            log_()->warn("Request " + std::to_string(pair.first) + " has been stuck in state '" + 
//                         info.status + "' for " + std::to_string(duration) + " seconds");
//        }
//    }
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

std::string IPC::formatRequest(const std::string& request, uint64_t id) {
    log_()->debug("Entering formatRequest for ID: " + std::to_string(id));
    
    std::string message = request + "\n" + std::to_string(id);
    size_t messageLength = message.length();
    
    log_()->debug("Message length: " + std::to_string(messageLength));

    std::ostringstream idStream;
    idStream << std::setw(8) << std::setfill('0') << (id % 100000000);
    std::string paddedId = idStream.str();
    
    log_()->debug("Padded ID: " + paddedId);

    std::ostringstream markerStream;
    markerStream << "START_" << paddedId << std::setw(8) << std::setfill('0') << messageLength;
    std::string start_marker = markerStream.str();
    
    log_()->debug("Start marker: " + start_marker);

    std::string formattedRequest = start_marker + message;
    log_()->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "...");

    return formattedRequest;
}

bool IPC::writeRequest(const std::string& message, ResponseCallback callback) {
    log_()->debug("Entering writeRequest method");
    
    std::lock_guard<std::mutex> lock(mutex_);
    log_()->debug("Mutex acquired in writeRequest");

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t id = nextRequestId_++;
    log_()->debug("Generated request ID: " + std::to_string(id));
    
    std::string formattedRequest;
    try {
        formattedRequest = formatRequest(message, id);
        log_()->debug("Request formatted successfully");
    } catch (const std::exception& e) {
        log_()->error("Exception in formatRequest: " + std::string(e.what()));
        return false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    log_()->debug("Time taken to format request: " + std::to_string(duration.count()) + "ms");

    if (duration > std::chrono::seconds(5)) {
        log_()->warn("Request formatting took longer than 5 seconds");
    }

    if (callback) {
        log_()->debug("Storing callback for request ID: " + std::to_string(id));
        pendingCallbacks_[id] = std::move(callback);
    } else {
        log_()->debug("No callback provided for request ID: " + std::to_string(id));
    }
    
    log_()->debug("Pushing request to queue. Current queue size: " + std::to_string(requestQueue_.size()));
    requestQueue_.push(formattedRequest);
    log_()->debug("Request pushed. New queue size: " + std::to_string(requestQueue_.size()));
    
    cv_.notify_one();
    log_()->debug("Condition variable notified");
    
    log_()->debug("Exiting writeRequest method");
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

void IPC::processResponse(const std::string& response) {
	size_t idPos = response.find_last_of('\n');
	if (idPos == std::string::npos) {
		log_()->error("Invalid response format");
		return;
	}
	uint64_t id = std::stoull(response.substr(idPos + 1));
	std::string actualResponse = response.substr(0, idPos);

	trackRequest(id, "responded");

	ResponseCallback callback;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = pendingCallbacks_.find(id);
		if (it == pendingCallbacks_.end()) {
			// No callback for this ID, which is fine for requests without callbacks
			return;
		}
		callback = it->second;
		pendingCallbacks_.erase(it);
	}
	callback(actualResponse);
}

void IPC::requestWriter() {
    log_()->debug("requestWriter thread started");
    while (!shouldStop_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !requestQueue_.empty() || shouldStop_; });

        if (shouldStop_) {
			log_()->debug("requestWriter thread stopping");
			break;
		}

        std::string request = requestQueue_.front();
        requestQueue_.pop();
        lock.unlock();

		log_()->debug("Attempting to write request: " + request);
        if (!writeRequestInternal(request)) {
            log_()->error("Failed to write request: " + request);
            // Handle error, possibly by requeueing the request or notifying the callback of failure
        } else {
            log_()->debug("Successfully wrote request: " + request);
        }
    }
}

void IPC::responseReader() {
    while (!shouldStop_) {
        std::string response = readResponseInternal(pipes_[responsePipePath]);
        if (!response.empty()) {
            processResponse(response);
        }
        // Add a small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool IPC::writeRequestInternal(const std::string& message) {
	// Extract the request ID from the formatted message
	size_t idStart = message.find("START_") + 6;
	size_t idEnd = message.find_first_of('_', idStart);
	uint64_t id = std::stoull(message.substr(idStart, idEnd - idStart));


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

	trackRequest(id, "written");
	log_()->info("Message: (" + message + ") written to request pipe");
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
