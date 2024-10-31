#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ResponseParser.h"
#include "Types.h"

TEST_CASE("ResponseParser::parsePlugins") {
    ResponseParser parser;

    SUBCASE("Parse single plugin") {
        std::string input = "1,TestPlugin,#AUv2test.uri";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 1);
        CHECK(plugins[0].number == 1);
        CHECK(plugins[0].name == "TestPlugin");
        CHECK(plugins[0].type == "AUv2");
        CHECK(plugins[0].uri == "test.uri");
    }

    SUBCASE("Parse multiple plugins") {
        std::string input = "1,Plugin1,#AUv2uri1|2,Plugin2,#VSTuri2|3,Plugin3,#AUv3uri3";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 3);
        CHECK(plugins[0].name == "Plugin1");
        CHECK(plugins[1].name == "Plugin2");
        CHECK(plugins[2].name == "Plugin3");
    }

    SUBCASE("Handle empty input") {
        std::string input = "";
        auto plugins = parser.parsePlugins(input);

        CHECK(plugins.empty());
    }

    SUBCASE("Handle malformed input") {
        std::string input = "1,IncompletePlugin";
        auto plugins = parser.parsePlugins(input);

        CHECK(plugins.empty());
    }

    SUBCASE("Sort plugins alphabetically") {
        std::string input = "2,ZPlugin,#AUv2uri2|1,APlugin,#VSTuri1|3,MPlugin,#AUv3uri3";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 3);
        CHECK(plugins[0].name == "APlugin");
        CHECK(plugins[1].name == "MPlugin");
        CHECK(plugins[2].name == "ZPlugin");
    }

    SUBCASE("Handle duplicate plugins with different types") {
        std::string input = "1,DupePlugin,#AUv2uri1|2,DupePlugin,#VSTuri2|3,UniquePlugin,#AUv3uri3";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 2);
        CHECK(plugins[0].name == "DupePlugin");
        CHECK(plugins[0].type == "AUv2");  // AUv2 should be preferred over VST
        CHECK(plugins[1].name == "UniquePlugin");
    }
}
