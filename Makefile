CC = clang++
CXXFLAGS = -std=c++17 -dynamiclib
INCLUDE = -I./src \
					-I./src/lib/platform/macos
LIBS = -lc++ -lSystem

FRAMEWORKS = -framework Cocoa -framework CoreFoundation -framework CoreGraphics

# Paths
SRC_DIR = ./src
BUILD_DIR = ./build
DYLIB = $(BUILD_DIR)/LiveImproved.dylib

# Sources
SRC = \
    $(SRC_DIR)/Main.mm \
    $(SRC_DIR)/lib/ApplicationManager.cpp \
    $(SRC_DIR)/lib/LogHandler.cpp \
    $(SRC_DIR)/lib/platform/macos/GUISearchBox.mm \
    $(SRC_DIR)/lib/platform/macos/IPC.cpp \
    $(SRC_DIR)/lib/platform/macos/PID.mm \
    $(SRC_DIR)/lib/platform/macos/EventHandler.mm \
    $(SRC_DIR)/lib/platform/macos/KeySender.mm \
    $(SRC_DIR)/lib/ActionHandler.cpp \
    $(SRC_DIR)/lib/ResponseParser.cpp

# Qt
QT_PATH = $(HOME)/Qt/6.7.2/macos
MOC = $(QT_PATH)/libexec/moc
MACDEPLOYQT = $(QT_PATH)/bin/macdeployqt

MOC_HEADERS = $(SRC_DIR)/lib/qt/Test.h
MOC_SOURCES = $(MOC_HEADERS:.h=.moc.cpp)
APP_SOURCES = $(SRC) $(MOC_SOURCES)

# explicitly list so clean doesn't nuke .mm's
APP_OBJECTS = \
    $(SRC_DIR)/Main.o \
    $(SRC_DIR)/lib/ApplicationManager.o \
    $(SRC_DIR)/lib/LogHandler.o \
    $(SRC_DIR)/lib/platform/macos/GUISearchBox.o \
    $(SRC_DIR)/lib/platform/macos/IPC.o \
    $(SRC_DIR)/lib/platform/macos/PID.o \
    $(SRC_DIR)/lib/platform/macos/EventHandler.o \
    $(SRC_DIR)/lib/platform/macos/KeySender.o \
    $(SRC_DIR)/lib/ActionHandler.o \
    $(SRC_DIR)/lib/ResponseParser.o \
    $(MOC_SOURCES:.cpp=.o)

# application-specific
APP_TARGET = LiveImproved
BUNDLE_PATH = ./build/$(TARGET).app

# Targets
lib: $(DYLIB)
app: $(APP)
inject: $(DYLIB)
	@export DYLD_INSERT_LIBRARIES=$(DYLIB) && \
	open /Applications/Ableton\ Live\ 12\ Suite.app && \
	unset DYLD_INSERT_LIBRARIES

# Rule to build the dynamic library
$(DYLIB): $(SRC)
	$(CC) $(CXXFLAGS) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FRAMEWORKS)

# build target application
$(BUNDLE_PATH): $(APP_OBJECTS)
	$(CXX) $(APP_OBJECTS) -o $(APP_TARGET) $(LDFLAGS)
	mkdir -p $(BUNDLE_PATH)/Contents/MacOS
	mv $(APP_TARGET) $(BUNDLE_PATH)/Contents/MacOS/
	cp -R $(QT_PATH)/lib/QtWidgets.framework $(BUNDLE_PATH)/Contents/MacOS/
	cp -R $(QT_PATH)/lib/QtCore.framework $(BUNDLE_PATH)/Contents/MacOS/
	install_name_tool -add_rpath @executable_path/../Frameworks $(BUNDLE_PATH)/Contents/MacOS/$(APP_TARGET)
	$(MACDEPLOYQT) $(BUNDLE_PATH)

# Rule to build the object files
%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CC) $(CXXFLAGS) -c $< -o $@

# Rule to generate MOC files
$(MOC_SOURCES): %.moc.cpp: %.h
	$(MOC) $< -o $@

# Clean rule
clean:
	rm -f $(DYLIB)
	rm -f $(APP_OBJECTS)
	rm -f $(APP_TARGET)
	rm -rf $(BUNDLE_PATH)
	rm -f $(MOC_SOURCES) $(MOC_SOURCES:.moc.cpp=.o)

# Run the application
run: $(BUNDLE_PATH)
	open $(BUNDLE_PATH)

inject: 

## Post-build actions
#post-build:
#	@export DYLD_INSERT_LIBRARIES=$(DYLIB) && \
#	 open /Applications/Ableton\ Live\ 12\ Suite.app && \
#	 unset DYLD_INSERT_LIBRARIES
#
## Default rule
#.PHONY: all clean
#all: $(DYLIB) post-build

# Clean rule
