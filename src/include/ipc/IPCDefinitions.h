#pragma once
#include <chrono>
#include <filesystem>
#include <optional>
#include <queue>
#include <sstream>

#include "LogGlobal.h"

namespace ipc {
    #ifdef _WIN32
    struct HANDLE__;
    using Handle = struct HANDLE__*;
    static constexpr Handle UNINITIALIZED_HANDLE = nullptr;
    #else
    using Handle = int;
    static constexpr Handle NULL_PIPE_HANDLE = -69420;
    static constexpr Handle INVALID_PIPE_HANDLE = -1;
    #endif

    using Path = std::filesystem::path;

    static constexpr mode_t     DEFAULT_DIRECTORY_PERMISSIONS  {0777};
    static constexpr mode_t     DEFAULT_PIPE_PERMISSIONS       {0666};

    static constexpr int    MAX_READ_RETRIES           {100};
    static constexpr int    MESSAGE_TRUNCATE_CHARS     {100};
    static constexpr int    MAX_PIPE_CREATION_ATTEMPTS {100};
    static constexpr int    MAX_PIPE_SETUP_ATTEMPTS    {100};
    static constexpr size_t BUFFER_SIZE                {8192};

    using ms = std::chrono::milliseconds;
    // TODO: increasing delay causes buffer issues
    static constexpr ms     DELAY_BETWEEN_READS        {50};
    static constexpr ms     PIPE_CREATION_RETRY_DELAY  {500};
    static constexpr ms     PIPE_SETUP_RETRY_DELAY     {500};
    static constexpr ms     LIVE_TICK                  {100};

    static constexpr std::string_view START_MARKER      {"START_"};
    static constexpr std::string_view END_MARKER        {"END_OF_MESSAGE"};
    static constexpr size_t           START_MARKER_SIZE {6};
    static constexpr size_t           REQUEST_ID_SIZE   {8};
    static constexpr size_t           HEADER_SIZE       {START_MARKER_SIZE + REQUEST_ID_SIZE + REQUEST_ID_SIZE};

    using ResponseCallback = std::optional<std::function<void(const std::string&)>>;

    enum class QueueState {
        Initializing
        , Running
        , Halted
        , Processing
    };
    enum class PipeState {
        Ready
        , Reading
        , Writing
        , Error
        , Closed
    };

    enum class ResponseType {
        Success
        , Error
    };

    struct PipeResponse {
        ipc::ResponseType returnType;
        ipc::Handle pipeHandle;
        std::optional<std::string> errorStr;
        
        // Constructor for success
        PipeResponse(ResponseType type, int handle)
            : returnType(type), pipeHandle(handle), errorStr(std::nullopt) {}

        // Constructor for error
        PipeResponse(ResponseType type, const std::string& error)
            : returnType(type), pipeHandle(-1), errorStr(error) {}
    };

    struct Request {
        std::string message;
        ipc::ResponseCallback callback;

        Request(std::string msg, ipc::ResponseCallback cb = std::nullopt)
            : message(std::move(msg)), callback(std::move(cb)) {}

        [[nodiscard]] auto formatted() -> std::string {
            uint64_t id = 1;
            //try {
            //    formattedRequest = formatRequest(request.message, id);
            //    logger->debug("Request formatted successfully");
            //} catch (const std::exception& e) {
            //    logger->error("Exception in formatRequest: " + std::string(e.what()));
            //    return false;
            //}
            size_t messageLength = message.length();

            std::ostringstream idStream;
            idStream << std::setw(8) << std::setfill('0') << (id % 100000000); // NOLINT - magic numbers
            std::string paddedId = idStream.str();

            std::ostringstream markerStream;
            markerStream << ipc::START_MARKER << paddedId << std::setw(8) << std::setfill('0') << messageLength; // NOLINT - magic numbers
            std::string start_marker = markerStream.str();

            std::string formattedRequest = start_marker + message;
            logger->debug("Formatted request (truncated): " + formattedRequest.substr(0, 50) + "..."); // NOLINT - magic numbers
            formattedRequest += "\n"; // add newline as a delimiter

            return formattedRequest;
        }

    };

    struct Response {
        ipc::ResponseType type;
        std::optional<std::string> data;

        Response(ipc::ResponseType t, std::optional<std::string> d)
            : type(t), data(std::move(d)) {}

        [[nodiscard]] auto success() const -> bool {
            bool isSuccess = (type == ipc::ResponseType::Success);
            logger->debug("Response success check: " + std::to_string(isSuccess) +
                          " (type: " + std::to_string(static_cast<int>(type)) + ")");
            return isSuccess;
        }
    };

    using RequestQueue = std::queue<Request>;

}
