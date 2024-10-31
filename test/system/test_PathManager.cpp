#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "PathManager.h"
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class TestPathManager : public PathManager {
public:
    using PathManager::PathManager;
    using PathManager::isValidFile;
    using PathManager::isValidDir;
};

TEST_CASE("PathManager basic functionality") {
    TestPathManager pm;

    SUBCASE("isValidFile and isValidDir") {
        // Create a temporary file and directory for testing
        fs::path tempDir = fs::temp_directory_path() / "test_pathmanager";
        fs::create_directory(tempDir);
        fs::path tempFile = tempDir / "test.txt";
        std::ofstream(tempFile.string()) << "Test content";

        CHECK(pm.isValidFile(tempFile));
        CHECK(pm.isValidDir(tempDir));
        CHECK_FALSE(pm.isValidFile(tempDir));
        CHECK_FALSE(pm.isValidDir(tempFile));

        // Clean up
        fs::remove(tempFile);
        fs::remove(tempDir);
    }

    SUBCASE("home directory") {
        fs::path homePath = pm.home();
        CHECK_FALSE(homePath.empty());
        CHECK(fs::exists(homePath));
        CHECK(fs::is_directory(homePath));
    }

    SUBCASE("documents directory") {
        fs::path docsPath = pm.documents();
        CHECK_FALSE(docsPath.empty());
        CHECK(fs::exists(docsPath));
        CHECK(fs::is_directory(docsPath));
    }

    SUBCASE("remote scripts directory") {
        fs::path remoteScripts = pm.remoteScripts();
        CHECK_FALSE(remoteScripts.empty());
        CHECK(fs::exists(remoteScripts));
        CHECK(fs::is_directory(remoteScripts));
        CHECK(remoteScripts.string().find("Remote Scripts") != std::string::npos);
    }
}

#ifdef _WIN32
TEST_CASE("Windows-specific PathManager tests") {
    TestPathManager pm;

    SUBCASE("localAppData") {
        fs::path localAppData = pm.localAppData();
        CHECK_FALSE(localAppData.empty());
        CHECK(fs::exists(localAppData));
        CHECK(fs::is_directory(localAppData));
    }
}
#else
// live binary
//TEST_CASE("macOS-specific PathManager tests") {
//    TestPathManager pm;
//
//    SUBCASE("liveBundle") {
//        // This test assumes Ableton Live is installed and running
//        // You might want to add a check to skip this test if Live is not running
//        CHECK_NOTHROW(pm.liveBundle());
//    }
//
//    SUBCASE("liveBinary") {
//        // This test assumes Ableton Live is installed
//        // You might want to add a check to skip this test if Live is not installed
//        fs::path liveBinary = pm.liveBinary();
//        CHECK_FALSE(liveBinary.empty());
//        CHECK(fs::exists(liveBinary));
//        CHECK(fs::is_regular_file(liveBinary));
//    }
//}
#endif
