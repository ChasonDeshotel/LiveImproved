#include "IPC.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include "../../ApplicationManager.h"
#include "IPC.h"

IPC::IPC(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
{}

IPC::~IPC() {
    for (auto& pipe : pipes_) {
        if (pipe.second != -1) {
            close(pipe.second);
        }
        unlink(pipe.first.c_str());
    }
}

bool IPC::init() {
    log_->info("IPC::init() called");

    removePipeIfExists(requestPipePath);
    removePipeIfExists(responsePipePath);

    if (!createPipe(requestPipePath) || !createPipe(responsePipePath)) {
        log_->info("IPC::init() failed");
        return false;
    }

    // make sure we can open/read the response pipe
    if (!openPipeForRead(responsePipePath, true)) {
        log_->info("IPC::init() failed to open pipes for initial communication");
        return false;
    }

    // Send "READY" signal to the Remote Script
    //if (!writeRequest("READY")) {
    //    log_->info("IPC::init() failed to send READY signal");
    //    return false;
    //}

    // wait until the remote script is able to read
    // this is probably backwards because this blocks ableton from loading
    // or should be delayed
    std::string response;
    int max_attempts = 1000;
    int attempt = 0;
    while (attempt < max_attempts) {
        if (openPipeForWrite(requestPipePath, true)) {  // Try opening in non-blocking mode
            log_->info("Request pipe successfully opened for writing");
            break;
        }

        log_->info("Attempt " + std::to_string(attempt + 1) + " to open request pipe for writing failed. Retrying...");
        usleep(100000); // 100ms
        attempt++;
    }

    if (pipes_[requestPipePath] == -1) {
        log_->info("IPC::init() could not open request pipe after " + std::to_string(max_attempts) + " attempts");
        return false;
    }

    log_->info("IPC::init() finished");
    return true;
}

void IPC::removePipeIfExists(const std::string& pipe_name) {
    if (access(pipe_name.c_str(), F_OK) != -1) {
        // File exists, so remove it
        if (unlink(pipe_name.c_str()) == 0) {
            log_->info("Removed existing pipe: " + pipe_name);
        } else {
            log_->info("Failed to remove existing pipe: " + pipe_name + " - " + strerror(errno));
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
            log_->info("Failed to create directory: " + directory + " - " + strerror(errno));
            return false;
        }
    }

    // Now create the pipe
    if (mkfifo(pipe_name.c_str(), 0666) == -1) {
        if (errno == EEXIST) {
            log_->info("Pipe already exists: " + pipe_name);
        } else {
            log_->info("Failed to create named pipe: " + pipe_name + " - " + strerror(errno));
            return false;
        }
    } else {
        log_->info("Pipe created: " + pipe_name);
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
        log_->info("Failed to open pipe for writing: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    log_->info("Pipe opened for writing: " + pipe_name);
    return true;
}

bool IPC::openPipeForRead(const std::string& pipe_name, bool non_blocking) {
    int flags = O_RDONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        log_->info("Failed to open pipe for reading: " + pipe_name + " - " + strerror(errno));
        return false;
    }

    pipes_[pipe_name] = fd;
    log_->info("Pipe opened for reading: " + pipe_name);
    return true;
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
        log_->info("Request pipe not opened for writing: " + requestPipePath);
        return false;
    }

    ssize_t result = write(fd, message.c_str(), message.length());
    if (result == -1) {
        if (errno == EAGAIN) {
            log_->info("Request pipe is full, message could not be written: " + std::string(strerror(errno)));
        } else {
            log_->info("Failed to write to request pipe: " + requestPipePath + " - " + strerror(errno));
        }
        return false;
    }

    log_->info("Message written to request pipe: " + requestPipePath);
    return true;
}

std::string IPC::readResponse() {
    if (pipes_[responsePipePath] == -1) {
        if (!openPipeForRead(responsePipePath, true)) {  // Open in non-blocking mode
            return "";
        }
    }

    char buffer[1024];
    std::string result;

    int fd = pipes_[responsePipePath];
    if (fd == -1) {
        log_->info("Response pipe not opened for reading: " + responsePipePath);
        return result;
    }

    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        if (errno == EAGAIN) {
            log_->info("No data available in the response pipe for reading: " + std::string(strerror(errno)));
        } else {
            log_->info("Failed to read from response pipe: " + responsePipePath + " - " + strerror(errno));
        }
    } else if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        result = std::string(buffer);
        log_->info("Message read from response pipe: " + responsePipePath);
    }

    return result;
}
