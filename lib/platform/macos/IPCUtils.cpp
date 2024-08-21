#include "IPCUtils.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

IPCUtils::IPCUtils() = default;

IPCUtils::~IPCUtils() {
    for (auto& pipe : pipes_) {
        close(pipe.second);
        unlink(pipe.first.c_str());
    }
}

void IPCUtils::createPipe(const std::string& pipe_name) {
    if (mkfifo(pipe_name.c_str(), 0666) == -1 && errno != EEXIST) {
        std::cerr << "Failed to create named pipe: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    pipes_[pipe_name] = -1;
}

void IPCUtils::openPipeForWrite(const std::string& pipe_name, bool non_blocking) {
    int flags = O_WRONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        std::cerr << "Failed to open pipe for writing: " << strerror(errno) << std::endl;
        return;
    }

    pipes_[pipe_name] = fd;
}

void IPCUtils::openPipeForRead(const std::string& pipe_name, bool non_blocking) {
    int flags = O_RDONLY;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }

    int fd = open(pipe_name.c_str(), flags);
    if (fd == -1) {
        std::cerr << "Failed to open pipe for reading: " << strerror(errno) << std::endl;
        return;
    }

    pipes_[pipe_name] = fd;
}

void IPCUtils::writeToPipe(const std::string& pipe_name, const std::string& message) {
    int fd = pipes_[pipe_name];
    if (fd == -1) {
        std::cerr << "Pipe not opened for writing: " << pipe_name << std::endl;
        return;
    }

    ssize_t result = write(fd, message.c_str(), message.length());
    if (result == -1) {
        if (errno == EAGAIN) {
            std::cerr << "Pipe is full, message could not be written: " << strerror(errno) << std::endl;
        } else {
            std::cerr << "Failed to write to pipe: " << strerror(errno) << std::endl;
        }
    }
}

std::string IPCUtils::readFromPipe(const std::string& pipe_name) {
    char buffer[1024];
    std::string result;

    int fd = pipes_[pipe_name];
    if (fd == -1) {
        std::cerr << "Pipe not opened for reading: " << pipe_name << std::endl;
        return result;
    }

    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        if (errno == EAGAIN) {
            std::cerr << "No data available in the pipe for reading: " << strerror(errno) << std::endl;
        } else {
            std::cerr << "Failed to read from pipe: " << strerror(errno) << std::endl;
        }
    } else if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        result = std::string(buffer);
    }

    return result;
}
