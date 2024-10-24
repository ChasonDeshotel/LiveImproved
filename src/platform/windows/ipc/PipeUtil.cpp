#include <windows.h>

#include "LogGlobal.h"
#include "IPCDefinitions.h"
#include "PipeUtil.h"

PipeUtil::PipeUtil()
    : pipeHandle_(ipc::INVALID_PIPE_HANDLE)
    , pipePath_()
    , pipeAccess_()
{}

auto PipeUtil::createPipe() -> bool {
    logger->error("pipe name: " + getPath().string());
    if (pipeAccess_ == ipc::PipeAccess::Read) {
        pipeHandle_ = CreateNamedPipeA(
            pipePath_.c_str()
            , PIPE_ACCESS_INBOUND  // Server will read from this pipe
            , PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
            , PIPE_UNLIMITED_INSTANCES
            , ipc::BUFFER_SIZE
            , ipc::BUFFER_SIZE
            , 0
            , NULL
        );
    } else if (pipeAccess_ == ipc::PipeAccess::Write) {
        pipeHandle_ = CreateNamedPipeA(
            pipePath_.c_str()
            , PIPE_ACCESS_OUTBOUND  // Server will write to this pipe
            , PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
            , PIPE_UNLIMITED_INSTANCES
            , ipc::BUFFER_SIZE
            , ipc::BUFFER_SIZE
            , 0,
            , NULL
        );
    }

    if (pipeHandle_ == ipc::INVALID_HANDLE_VALUE) {
        logger->error("Failed to create pipe: " + pipe_name);
        return false;
    }
}

// connect to pipe
auto PipeUtil::openPipe(const std::string& pipe_name) -> bool {
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

auto PipeUtil::closePipe() -> bool {
    if (pipeHandle_ != INVALID_HANDLE_VALUE) {
        if (!CloseHandle(pipeHandle_)) {
            DWORD error = GetLastError();
            logger->error("Failed to close pipe handle. Error code: " + std::to_string(error));
            //throw std::runtime_error("Failed to close pipe handle. Error code: " + std::to_string(error));
            return false;

        }
        pipeHandle_ = INVALID_HANDLE_VALUE;
    }
    return true;
}

auto IPCUtil::deletePipe() -> void {
//    // No equivalent of `unlink` for named pipes on Windows, so this is a no-op.
//    logger->debug("No need to remove pipes on Windows: " + pipe_name);
    return true;
}

auto PipeUtil::ensurePipeOpen() -> bool {
    return true;
}

void IPCUtil::drainPipe() {
    char buffer[ipc::BUFFER_SIZE];
    DWORD bytesRead;
    do {
        ReadFile(pipeHandle_, buffer, sizeof(buffer), &bytesRead, NULL);
    } while (bytesRead > 0);
}

auto PipeUtil::writeToPipe(const ipc::Request& request) -> size_t {
    const std::string& formattedRequest = request.formatted();
    DWORD bytesWritten = 0;
    BOOL success = WriteFile(
        getHandle()
        , formattedRequest.c_str()
        , static_cast<DWORD>(formattedRequest.length())
        , &bytesWritten
        , NULL // Not using overlapped I/O
    );

    if (!success) {
        DWORD error = GetLastError();
        if (error == ERROR_NO_DATA) {
            logger->error("Request pipe is broken or closed, message could not be written.");
        } else {
            logger->error("Failed to write to request pipe: " + getPath().string() + " - Error code: " + std::to_string(error));
        }
        throw std::runtime_error("Write to pipe failed");
    } else if (bytesWritten != formattedRequest.length()) {
        logger->error("Incomplete write to request pipe. Wrote " + std::to_string(bytesWritten) + " of " + std::to_string(formattedRequest.length()) + " bytes");
        throw std::runtime_error("Incomplete write to pipe");
    }

    logger->debug("Request written successfully, bytes written: " + std::to_string(bytesWritten));
    return static_cast<size_t>(bytesWritten);
}

auto PipeUtil::readFromPipe(void* buffer, size_t count) const -> ssize_t {
    DWORD bytesRead = 0;
    BOOL success = ReadFile(
        getHandle()
        , buffer
        , static_cast<DWORD>(count)
        , &bytesRead
        , NULL // Not using overlapped I/O
    );

    if (!success) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            // The pipe has been ended - this is not always an error
            return 0;
        } else {
            throw std::runtime_error("Read error: " + std::to_string(error));
        }
    }

    return static_cast<ssize_t>(bytesRead);
}
