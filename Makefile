CC = clang++

CXXFLAGS = -std=c++20                             \
         -DJUCE_APP_CONFIG_HEADER=\"$(PWD)/src/include/AppConfig.h\" \
         -DDEBUG                                  \
				 -ObjC++                                  \
         -arch x86_64
	
INCLUDE =                                                   \
        -I./mock                                            \
        -I./src/include                                     \
				-I/usr/local/Cellar/harfbuzz/9.0.0/include/harfbuzz \
				-I/usr/local/include

INCLUDE_JUCE = -I$(JUCE_DIR)                    \
             -I$(JUCE_DIR)/juce_core            \
             -I$(JUCE_DIR)/juce_gui_basics      \
             -I$(JUCE_DIR)/juce_graphics        \
             -I$(JUCE_DIR)/juce_events          \
             -I$(JUCE_DIR)/juce_data_structures

OBJ_DIR = $(BUILD_DIR)/obj

LIBS =                                      \
     -lc++                                  \
     -lSystem                               \
		 -L/usr/local/Cellar/yaml-cpp/0.8.0/lib \
     -lyaml-cpp                             \
     -lharfbuzz

FRAMEWORKS =                              \
           -framework Cocoa               \
           -framework CoreFoundation      \
           -framework CoreGraphics        \
           -framework ApplicationServices \
           -framework CoreVideo           \
           -framework IOKit               \
           -framework CoreImage           \
           -framework Security            \
           -framework QuartzCore

SRC_DIR       = ./src
BUILD_DIR     = ./build
JUCE_DIR      = ./juce/modules

# building as library
DYLIB         = $(BUILD_DIR)/LiveImproved.dylib

# building as application
APP_TARGET     = LiveImproved
BUNDLE_PATH    = $(BUILD_DIR)/$(APP_TARGET).app
APP_EXECUTABLE = $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)
PLIST_PATH     = $(BUNDLE_PATH)/Contents/Info.plist
LICENSE_PATH   = $(BUNDLE_PATH)/Contents/LICENSE

LIVE          = /Applications/Ableton\ Live\ 12\ Suite.app

MODULES =                                \
    Main.cpp                             \
    ApplicationManager.cpp               \
    LogHandler.cpp                       \
    WindowManager.cpp                    \
    config/ConfigManager.cpp             \
    config/ConfigMenu.cpp                \
    gui/SearchBox.cpp                    \
    platform/macos/PlatformInitalizer.mm \
    platform/macos/ContextMenu.mm        \
    platform/macos/IPC.cpp               \
    platform/macos/PID.mm                \
    platform/macos/EventHandler.mm       \
    platform/macos/KeySender.mm          \
    ActionHandler.cpp                    \
    ResponseParser.cpp                   \

JUCE_OBJS = $(OBJ_DIR)/juce_core/juce_core.o                       \
            $(OBJ_DIR)/juce_gui_basics/juce_gui_basics.o           \
            $(OBJ_DIR)/juce_graphics/juce_graphics.o               \
            $(OBJ_DIR)/juce_events/juce_events.o                   \
            $(OBJ_DIR)/juce_data_structures/juce_data_structures.o \
            $(OBJ_DIR)/juce_core/juce_core_CompilationTime.o

# for running tests
TEST_DIR      = ./test
BOOST_LIBS    = -lboost_unit_test_framework
BOOST_INCLUDE = /usr/local/Cellar/boost/1.86.0/include/
TEST_SRC      = $(wildcard $(TEST_DIR)/*.cpp)

# Ensure build directories exist
create_dirs: $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)/config
	mkdir -p $(OBJ_DIR)/include
	mkdir -p $(OBJ_DIR)/gui
	mkdir -p $(OBJ_DIR)/platform/macos
	mkdir -p $(OBJ_DIR)/config
	mkdir -p $(OBJ_DIR)/test
	mkdir -p $(OBJ_DIR)/juce_core
	mkdir -p $(OBJ_DIR)/juce_gui_basics
	mkdir -p $(OBJ_DIR)/juce_graphics
	mkdir -p $(OBJ_DIR)/juce_events
	mkdir -p $(OBJ_DIR)/juce_data_structures

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

# explicitly list so clean doesn't nuke .mm's
APP_OBJECTS =                                       \
    $(OBJ_DIR)/Main.o                               \
    $(OBJ_DIR)/ApplicationManager.o                 \
    $(OBJ_DIR)/LogHandler.o                         \
    $(OBJ_DIR)/WindowManager.o                      \
    $(OBJ_DIR)/config/ConfigManager.o               \
    $(OBJ_DIR)/config/ConfigMenu.o                  \
    $(OBJ_DIR)/gui/SearchBox.o                      \
    $(OBJ_DIR)/platform/macos/PlatformInitializer.o \
    $(OBJ_DIR)/platform/macos/ContextMenu.o         \
    $(OBJ_DIR)/platform/macos/IPC.o                 \
    $(OBJ_DIR)/platform/macos/PID.o                 \
    $(OBJ_DIR)/platform/macos/EventHandler.o        \
    $(OBJ_DIR)/platform/macos/KeySender.o           \
    $(OBJ_DIR)/ActionHandler.o                      \
    $(OBJ_DIR)/ResponseParser.o                     \
		$(JUCE_OBJS)
#    $(MOC_OBJECTS)
#    $(MOC_SOURCES:.cpp=.o)



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


juce: $(APP_OBJECTS) generate-plist
	@echo 'app obj'
	$(CC) $(APP_OBJECTS) -o $(APP_TARGET) $(CXXFLAGS) $(INCLUDE) $(LIBS) $(FRAMEWORKS) $(INCLUDE_JUCE) -headerpad_max_install_names
	mkdir -p $(BUNDLE_PATH)/Contents/MacOS
	mv $(APP_TARGET) $(BUNDLE_PATH)/Contents/MacOS/
	cp LICENSE $(LICENSE_PATH)
	install_name_tool -add_rpath @executable_path/../Frameworks $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)

$(OBJ_DIR)/juce_core/juce_core.o: $(JUCE_DIR)/juce_core/juce_core.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/juce_core/juce_core_CompilationTime.o: $(JUCE_DIR)/juce_core/juce_core_CompilationTime.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/juce_events/juce_events.o: $(JUCE_DIR)/juce_events/juce_events.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/juce_gui_basics/juce_gui_basics.o: $(JUCE_DIR)/juce_gui_basics/juce_gui_basics.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/juce_graphics/juce_graphics.o: $(JUCE_DIR)/juce_graphics/juce_graphics.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/juce_data_structures/juce_data_structures.o: $(JUCE_DIR)/juce_data_structures/juce_data_structures.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

# Rule to build the object files
# Define build directory creation as a prerequisite for object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | create_dirs
	@echo 'Compiling $< to $@'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.mm | create_dirs
	@echo 'o'
	$(CC) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_JUCE) -c $< -o $@

# Rule to compile .cpp files to .o files in test
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.cpp | create_dirs
	$(CC) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

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

test_config_manager: $(OBJ_DIR)/config/ConfigManager.o $(OBJ_DIR)/test/config_ConfigManager.o
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/test_config_manager $(OBJ_DIR)/config/ConfigManager.o $(OBJ_DIR)/test/config_ConfigManager.o $(INCLUDE) $(LIBS) $(BOOST_LIBS) $(FRAMEWORKS)


.PHONY: FORCE
FORCE:

run_tests: test_config_manager
	./$(BUILD_DIR)/test_config_manager --log_level=all

