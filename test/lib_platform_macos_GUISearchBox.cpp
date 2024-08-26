#define BOOST_TEST_MODULE GUISearchBoxTest
#include <boost/test/included/unit_test.hpp>
#include "GUISearchBox.h"
#include "ApplicationManager.h"

// Mock class for ApplicationManager since it's not the focus of our test
class MockApplicationManager : public ApplicationManager {
public:
    MockApplicationManager() : ApplicationManager() {}
    // Add any mock-specific methods if needed
};

struct GUISearchBoxFixture {
    MockApplicationManager mockAppManager;
    GUISearchBox* searchBox;

    GUISearchBoxFixture() {
        searchBox = new GUISearchBox(mockAppManager);
    }

    ~GUISearchBoxFixture() {
        delete searchBox;
    }
};

BOOST_FIXTURE_TEST_SUITE(GUISearchBoxTests, GUISearchBoxFixture)

BOOST_AUTO_TEST_CASE(InitializationTest) {
    BOOST_CHECK_EQUAL(searchBox->isOpen(), false);
    BOOST_CHECK_EQUAL(searchBox->getSearchText(), "");
}

BOOST_AUTO_TEST_CASE(SetSearchTextTest) {
    std::string text = "test text";
    searchBox->setSearchText(text);
    BOOST_CHECK_EQUAL(searchBox->getSearchText(), text);
}

BOOST_AUTO_TEST_CASE(ClearSearchTextTest) {
    searchBox->setSearchText("some text");
    searchBox->clearSearchText();
    BOOST_CHECK_EQUAL(searchBox->getSearchText(), "");
}

BOOST_AUTO_TEST_CASE(OpenCloseSearchBoxTest) {
    searchBox->openSearchBox();
    BOOST_CHECK_EQUAL(searchBox->isOpen(), true);

    searchBox->closeSearchBox();
    BOOST_CHECK_EQUAL(searchBox->isOpen(), false);
}

BOOST_AUTO_TEST_CASE(SetOptionsTest) {
    std::vector<Plugin> options = {
        {"Plugin 1"},
        {"Plugin 2"}
    };
    searchBox->setOptions(options);

    // Additional checks could be done here if you had a method to retrieve the options
}

BOOST_AUTO_TEST_SUITE_END()
