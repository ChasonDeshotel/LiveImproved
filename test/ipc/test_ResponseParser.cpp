#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ResponseParser.h"
#include "Types.h"

TEST_CASE("ResponseParser::parsePlugins") {
    ResponseParser parser;

    SUBCASE("Parse single plugin") {
        std::string input = "0,ShapeShifter,query:Plugins#AUv2:Aberrant%20DSP:ShapeShifter";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 1);
        CHECK(plugins[0].number == 1);
        CHECK(plugins[0].name == "ShapeShifter");
        CHECK(plugins[0].type == "AUv2");
        CHECK(plugins[0].uri == "Aberrant%20DSP:ShapeShifter");
    }

    SUBCASE("Parse single plugin with different type") {
        std::string input = "2,Virus TI,query:Plugins#VST3:Access:Virus%20TI";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 1);
        CHECK(plugins[0].number == 3);  // Remember, we're adding 1 to make it 1-based
        CHECK(plugins[0].name == "Virus TI");
        CHECK(plugins[0].type == "VST3");
        CHECK(plugins[0].uri == "Access:Virus%20TI");
    }

    SUBCASE("Parse multiple plugins") {
        std::string input = "0,ShapeShifter,query:Plugins#AUv2:Aberrant%20DSP:ShapeShifter|1,SketchCassette,query:Plugins#AUv2:Aberrant%20DSP:SketchCassette|2,Virus TI,query:Plugins#AUv2:Access:Virus%20TI";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 3);
        CHECK(plugins[0].name == "ShapeShifter");
        CHECK(plugins[1].name == "SketchCassette");
        CHECK(plugins[2].name == "Virus TI");
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
        std::string input = "0,DupePlugin,query:Plugins#AUv2:Aberrant%20DSP:ShapeShifter|1,DupePlugin,query:Plugins#VST3:Aberrant%20DSP:SketchCassette|2,UniquePlugin,query:Plugins#AUv2:Access:Virus%20TI";
        auto plugins = parser.parsePlugins(input);

        REQUIRE(plugins.size() == 2);
        CHECK(plugins[0].name == "DupePlugin");
        CHECK(plugins[0].type == "VST3");
        CHECK(plugins[1].name == "UniquePlugin");
    }
}
