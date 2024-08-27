CC = clang++

CXXFLAGS = -std=c++17

INCLUDE =                             \
        -I./mock                      \
				-I./src                       \
				-I./src/lib                   \
				-I./src/lib/platform/macos    \
				-I./src/lib/types

LIBS =                                \
     -lc++                            \
     -lSystem

FRAMEWORKS =                          \
           -framework Cocoa           \
           -framework CoreFoundation  \
           -framework CoreGraphics    \

SRC_DIR       = ./src
BUILD_DIR     = ./build

# building as library
DYLIB         = $(BUILD_DIR)/LiveImproved.dylib

# building as application
APP_TARGET     = LiveImproved
BUNDLE_PATH    = $(BUILD_DIR)/$(APP_TARGET).app
APP_EXECUTABLE = $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)

LIVE          = /Applications/Ableton\ Live\ 12\ Suite.app

MODULES =                               \
    Main.mm                            \
    lib/ApplicationManager.cpp         \
    lib/LogHandler.cpp                 \
    lib/platform/macos/GUISearchBox.mm \
    lib/platform/macos/IPC.cpp         \
    lib/platform/macos/PID.mm          \
    lib/platform/macos/EventHandler.mm \
    lib/platform/macos/KeySender.mm    \
    lib/ActionHandler.cpp              \
    lib/ResponseParser.cpp

# for running tests
TEST_DIR      = ./test
BOOST_LIBS    = -lboost_unit_test_framework
BOOST_INCLUDE = /usr/local/Cellar/boost/1.86.0/include/
TEST_SRC      = $(wildcard $(TEST_DIR)/*.cpp)

#
# for building with mock modules
# this will replace whatever normal modules
# with the mocked versions
# `make MOCK_MODULES="<mocked module>" <target>` 
#
# this can mock a .mm with a .cpp
# if .cpp and .mm exist, .cpp is used
MOCK_DIR = ./mock
SRC := $(foreach module,$(MODULES), \
    $(if $(filter $(basename $(notdir $(module))),$(MOCK_MODULES)), \
        $(if $(wildcard $(MOCK_DIR)/$(basename $(notdir $(module))).cpp), \
            $(MOCK_DIR)/$(basename $(notdir $(module))).cpp, \
            $(if $(wildcard $(MOCK_DIR)/$(basename $(notdir $(module))).mm), \
                $(MOCK_DIR)/$(basename $(notdir $(module))).mm, \
                $(SRC_DIR)/$(module))), \
        $(SRC_DIR)/$(module)))

#
# for building with Qt
# 
QT_PATH     = $(HOME)/Qt/6.7.2/macos
MOC         = $(QT_PATH)/libexec/moc
MACDEPLOYQT = $(QT_PATH)/bin/macdeployqt

MOC_HEADERS = $(SRC_DIR)/lib/qt/Test.h
MOC_SOURCES = $(MOC_HEADERS:.h=.moc.cpp)
APP_SOURCES = $(SRC) $(MOC_SOURCES)

# explicitly list so clean doesn't nuke .mm's
APP_OBJECTS =                                     \
    $(SRC_DIR)/Main.o                             \
    $(SRC_DIR)/lib/ApplicationManager.o           \
    $(SRC_DIR)/lib/LogHandler.o                   \
    $(SRC_DIR)/lib/platform/macos/GUISearchBox.o  \
    $(SRC_DIR)/lib/platform/macos/IPC.o           \
    $(SRC_DIR)/lib/platform/macos/PID.o           \
    $(SRC_DIR)/lib/platform/macos/EventHandler.o  \
    $(SRC_DIR)/lib/platform/macos/KeySender.o     \
    $(SRC_DIR)/lib/ActionHandler.o                \
    $(SRC_DIR)/lib/ResponseParser.o               \
    $(MOC_SOURCES:.cpp=.o)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#
# build library-style
#
lib: clean $(DYLIB)
$(DYLIB): $(SRC)
	$(CC) $(CXXFLAGS) -DINJECTED_LIBRARY -dynamiclib -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FRAMEWORKS)

app: clean $(BUNDLE_PATH)
	$(CC) $(CXXFLAGS) -o $(APP_EXECUTABLE) $(SRC) $(INCLUDE) $(LIBS) $(FRAMEWORKS)

$(BUNDLE_PATH):
	mkdir -p $(BUNDLE_PATH)/Contents/MacOS

inject: clean $(DYLIB)
	@export DYLD_INSERT_LIBRARIES=$(DYLIB) && \
	open $(LIVE) && \
	unset DYLD_INSERT_LIBRARIES

#
# build application-style
#
qtapp: clean $(APP_OBJECTS)
	$(CXX) $(APP_OBJECTS) -o $(APP_TARGET) $(LDFLAGS)
	mkdir -p $(BUNDLE_PATH)/Contents/MacOS
	mv $(APP_TARGET) $(BUNDLE_PATH)/Contents/MacOS/
	cp -R $(QT_PATH)/lib/QtWidgets.framework $(BUNDLE_PATH)/Contents/MacOS/
	cp -R $(QT_PATH)/lib/QtCore.framework $(BUNDLE_PATH)/Contents/MacOS/
	install_name_tool -add_rpath @executable_path/../Frameworks $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)
	$(MACDEPLOYQT) $(BUNDLE_PATH)

# 
# more Qt stuff
#
# Rule to build the object files
%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CC) $(CXXFLAGS) -c $< -o $@

$(MOC_SOURCES): %.moc.cpp: %.h
	$(MOC) $< -o $@

clean:
	rm -f $(DYLIB)
	rm -rf $(BUILD_DIR)/test
	rm -f $(APP_OBJECTS)
	rm -f $(APP_TARGET)
	rm -rf $(BUNDLE_PATH)
	rm -f $(MOC_SOURCES) $(MOC_SOURCES:.moc.cpp=.o)

run: $(BUNDLE_PATH)
	open $(BUNDLE_PATH)

test: FORCE
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/test $(TEST_SRC) $(INCLUDE) $(LIBS) $(BOOST_LIBS) $(FRAMEWORKS)

.PHONY: FORCE
FORCE:

run_tests: test
	$(BUILD_DIR)/test

