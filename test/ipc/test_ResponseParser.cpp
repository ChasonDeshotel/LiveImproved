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

    SUBCASE("Parse plugins with different types") {
        std::vector<std::string> inputs = {
            "0,ShapeShifter,query:Plugins#AUv2:Aberrant%20DSP:ShapeShifter",
            "1,Virus TI,query:Plugins#VST3:Access:Virus%20TI",
            "2,OldPlugin,query:Plugins#VST:Vintage:OldPlugin",
            "3,NewPlugin,query:Plugins#CLAP:Modern:NewPlugin"
        };

        for (const auto& input : inputs) {
            auto plugins = parser.parsePlugins(input);
            REQUIRE(plugins.size() == 1);
            const auto& plugin = plugins[0];

            if (plugin.name == "ShapeShifter") {
                CHECK(plugin.number == 1);
                CHECK(plugin.type == "AUv2");
                CHECK(plugin.uri == "Aberrant%20DSP:ShapeShifter");
            } else if (plugin.name == "Virus TI") {
                CHECK(plugin.number == 2);
                CHECK(plugin.type == "VST3");
                CHECK(plugin.uri == "Access:Virus%20TI");
            } else if (plugin.name == "OldPlugin") {
                CHECK(plugin.number == 3);
                CHECK(plugin.type == "VST");
                CHECK(plugin.uri == "Vintage:OldPlugin");
            } else if (plugin.name == "NewPlugin") {
                CHECK(plugin.number == 4);
                CHECK(plugin.type == "CLAP");
                CHECK(plugin.uri == "Modern:NewPlugin");
            }
        }
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
        std::string input = "0,ZPlugin,query:Plugins#AUv2:Aberrant%20DSP:ShapeShifter|1,APlugin,query:Plugins#AUv2:Aberrant%20DSP:SketchCassette|2,MPlugin,query:Plugins#AUv2:Access:Virus%20TI";
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
