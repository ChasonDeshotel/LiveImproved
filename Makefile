CC = clang++

CXXFLAGS = -std=c++17
	
INCLUDE =                             \
        -I./mock                      \
        -I./src                       \
        -I./src/lib                   \
        -I./src/lib/platform/macos    \
        -I./src/lib/types

INCLUDE_QT =                              \
           -I$(QT_PATH)/lib/QtWidgets.framework/Versions/A/Headers \
           -I$(QT_PATH)/lib/QtCore.framework/Versions/A/Headers    \
           -I ~/Qt/6.7.2/macos/lib/QtGui.framework/Headers \
           -I$(QT_PATH)/include           \
           -I$(QT_PATH)/include/QtWidgets \
           -I$(QT_PATH)/include/QtCore

OBJ_DIR = $(BUILD_DIR)/obj

LIBS =                                \
     -lc++                            \
     -lSystem

FRAMEWORKS =                          \
           -framework Cocoa           \
           -framework CoreFoundation  \
           -framework CoreGraphics

FRAMEWORKS_QT =                             \
              -F $(HOME)/Qt/6.7.2/macos/lib \
              -framework QtWidgets          \
              -framework QtCore             \
              -framework QtGui

SRC_DIR       = ./src
BUILD_DIR     = ./build

# building as library
DYLIB         = $(BUILD_DIR)/LiveImproved.dylib

# building as application
APP_TARGET     = LiveImproved
BUNDLE_PATH    = $(BUILD_DIR)/$(APP_TARGET).app
APP_EXECUTABLE = $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)
PLIST_PATH     = $(BUNDLE_PATH)/Contents/Info.plist

LIVE          = /Applications/Ableton\ Live\ 12\ Suite.app

MODULES =                               \
    Main.mm                            \
		lib/types/Plugin.h                 \
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

MOC_HEADERS = $(SRC_DIR)/lib/platform/macos/GUISearchBox.h \
							$(SRC_DIR)/Main.h
						#	$(SRC_DIR)/lib/platform/macos/EventHandlerThread.h

MOC_SOURCES = $(MOC_HEADERS:$(SRC_DIR)/%.h=$(OBJ_DIR)/%.moc.cpp)

MOC_OBJECTS = $(MOC_SOURCES:.cpp=.o)

# explicitly list so clean doesn't nuke .mm's
APP_OBJECTS =                                     \
    $(OBJ_DIR)/Main.o                             \
    $(OBJ_DIR)/lib/ApplicationManager.o           \
    $(OBJ_DIR)/lib/LogHandler.o                   \
    $(OBJ_DIR)/lib/platform/macos/GUISearchBox.o  \
    $(OBJ_DIR)/lib/platform/macos/IPC.o           \
    $(OBJ_DIR)/lib/platform/macos/PID.o           \
    $(OBJ_DIR)/lib/platform/macos/EventHandler.o  \
    $(OBJ_DIR)/lib/platform/macos/KeySender.o     \
    $(OBJ_DIR)/lib/ActionHandler.o                \
    $(OBJ_DIR)/lib/ResponseParser.o               \
    $(MOC_OBJECTS)
#    $(MOC_SOURCES:.cpp=.o)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)/lib/platform/macos


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
	# TODO: must be full path
	@export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/LiveImproved/build/LiveImproved.dylib && \
	open $(LIVE) && \
	unset DYLD_INSERT_LIBRARIES

#
# build application-style
#
generate-plist:
	@mkdir -p $(BUNDLE_PATH)/Contents
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $(PLIST_PATH)
	@echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(PLIST_PATH)
	@echo '<plist version="1.0">' >> $(PLIST_PATH)
	@echo '<dict>' >> $(PLIST_PATH)
	@echo '  <key>CFBundleName</key>' >> $(PLIST_PATH)
	@echo '  <string>$(APP_TARGET)</string>' >> $(PLIST_PATH)
	@echo '  <key>CFBundleIdentifier</key>' >> $(PLIST_PATH)
	@echo '  <string>com.example.$(APP_TARGET)</string>' >> $(PLIST_PATH)
	@echo '  <key>CFBundleVersion</key>' >> $(PLIST_PATH)
	@echo '  <string>1.0</string>' >> $(PLIST_PATH)
	@echo '  <key>CFBundleExecutable</key>' >> $(PLIST_PATH)
	@echo '  <string>$(APP_TARGET)</string>' >> $(PLIST_PATH)
	@echo '  <key>LSBackgroundOnly</key>' >> $(PLIST_PATH)
	@echo '  <string>1</string>' >> $(PLIST_PATH)
	@echo '</dict>' >> $(PLIST_PATH)
	@echo '</plist>' >> $(PLIST_PATH)

qtapp: clean $(APP_OBJECTS) generate-plist
	$(CC) $(APP_OBJECTS) -o $(APP_TARGET) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_QT) $(LIBS) $(FRAMEWORKS) $(FRAMEWORKS_QT) -headerpad_max_install_names
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
# Define build directory creation as a prerequisite for object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_QT) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.mm | $(BUILD_DIR)
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_QT) -c $< -o $@

# Rule to generate the .moc.cpp file from the .h file
$(OBJ_DIR)/%.moc.cpp: $(SRC_DIR)/%.h | $(BUILD_DIR)
	$(MOC) $< -o $@

# Rule to compile the .moc.cpp into a .o file
$(OBJ_DIR)/%.moc.o: $(OBJ_DIR)/%.moc.cpp
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_QT) -c $< -o $@

clean:
	rm -f $(DYLIB)
	rm -rf $(BUILD_DIR)/test
	rm -f $(APP_OBJECTS)
	rm -f $(APP_TARGET)
	rm -rf $(BUNDLE_PATH)
#	rm -f $(MOC_SOURCES) $(MOC_SOURCES:.moc.cpp=.o)

run: $(BUNDLE_PATH)
	$(APP_EXECUTABLE)
#	open $(BUNDLE_PATH)

test: FORCE
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/test $(TEST_SRC) $(INCLUDE) $(LIBS) $(BOOST_LIBS) $(FRAMEWORKS)

.PHONY: FORCE
FORCE:

run_tests: test
	$(BUILD_DIR)/test



