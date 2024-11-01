#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "PipeUtil.h"
#include "LogGlobal.h"

#include "IPCDefinitions.h"

namespace fs = std::filesystem;

TEST_CASE("global setup") {
    initializeLogger();
}

// in the application we have separate read and write pipes (request and response)
// but imo testing PipeUtil read and write on the same pipe (file path) is sufficient
// since we're still using the same logic to create/open/read/write/etc
//
// and we must set up a reader on the request pipe or we get a 'device not configured' error
//
// so this is way easier
//
TEST_CASE("PipeUtil initialization and basic operations") {
    SUBCASE("PipeUtil initialization and basic operations test") {
        fs::path testPipePath = fs::temp_directory_path() / "test_pipe";

        PipeUtil writePipe, readPipe;
        CHECK_NOTHROW(writePipe.setPath(testPipePath));
        CHECK_EQ(writePipe.getPath(), testPipePath);
        CHECK_EQ(writePipe.getHandle(), ipc::INVALID_PIPE_HANDLE);

        readPipe.setPath(testPipePath);

        writePipe.deletePipe();

        writePipe.setMode(ipc::PipeMode::Write);
        readPipe.setMode(ipc::PipeMode::Read);

        CHECK(writePipe.createPipe());

        std::this_thread::sleep_for(ipc::LIVE_TICK * 15);

        std::thread writeThread([&writePipe]() {
            std::cout << "Attempting to open pipe for write" << std::endl;
            CHECK(writePipe.openPipe());
        });

        std::thread readThread([&readPipe]() {
            std::cout << "Attempting to open pipe for read" << std::endl;
            CHECK(readPipe.openPipe());
        });

        writeThread.join();
        readThread.join();

        std::cout << "Write pipe handle: " << writePipe.getHandle() << std::endl;
        std::cout << "Read pipe handle: " << readPipe.getHandle() << std::endl;

        CHECK(writePipe.getHandle() != ipc::INVALID_PIPE_HANDLE);
        CHECK(readPipe.getHandle() != ipc::INVALID_PIPE_HANDLE);

        ipc::Request testRequest("TEST_COMMAND", {});
        testRequest.message = R"({"command":"TEST_COMMAND","args":["arg1","arg2"]})";
        size_t bytesWritten = writePipe.writeToPipe(testRequest);
        CHECK(bytesWritten > 0);

        // test reading
        char buffer[ipc::BUFFER_SIZE];
        ssize_t bytesRead = readPipe.readFromPipe(buffer, sizeof(buffer));
        CHECK(bytesRead > 0);

        writePipe.closePipe();
    }
}

TEST_CASE("PipeUtil error handling") {
    std::cout << "Starting PipeUtil error handling test" << std::endl;

    PipeUtil pipeUtil;
    fs::path nonExistentPath = "/path/that/does/not/exist/pipe";

    SUBCASE("Open non-existent pipe") {
        pipeUtil.setPath(nonExistentPath);
        CHECK_FALSE(pipeUtil.openPipe());
    }

    SUBCASE("Write to closed pipe") {
        pipeUtil.setPath(nonExistentPath);
        ipc::Request testRequest("TEST_COMMAND", {});
        CHECK_FALSE(pipeUtil.writeToPipe(testRequest));
    }

    SUBCASE("Read from closed pipe") {
        pipeUtil.setPath(nonExistentPath);
        char buffer[ipc::BUFFER_SIZE];
        CHECK_THROWS(pipeUtil.readFromPipe(buffer, sizeof(buffer)));
    }
}
