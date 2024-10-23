#pragma once
#include <chrono>
#include <filesystem>
#include <queue>

namespace ipc {
    enum class QueueState {
        Initializing
        , Running
        , Halted
        , Processing
        , Recovering
    };
    enum class PipeState {
        Ready
        , Reading
        , Writing
        , Error
        , Closed
    };

    using ResponseCallback = std::function<void(const std::string&)>;
    using Request = std::pair<std::string, ResponseCallback>;
    using RequestQueue = std::queue<Request>;

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
}
