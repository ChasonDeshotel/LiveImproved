CC = clang++
CXXFLAGS = -std=c++17 -dynamiclib
INCLUDE = -I./lib \
					-I./lib/platform/macos
LIBS = -lc++ -lSystem

FRAMEWORKS = -framework Cocoa -framework CoreFoundation -framework CoreGraphics

# Paths
SRC_DIR = ./lib
BUILD_DIR = ./build
DYLIB = $(BUILD_DIR)/LiveImproved.dylib

# Sources
SRC = \
    ./Main.mm \
    $(SRC_DIR)/ApplicationManager.cpp \
    $(SRC_DIR)/LogHandler.cpp \
    $(SRC_DIR)/platform/macos/GUISearchBox.mm \
    $(SRC_DIR)/platform/macos/IPC.cpp \
    $(SRC_DIR)/platform/macos/PID.mm \
    $(SRC_DIR)/platform/macos/EventHandler.mm \
    $(SRC_DIR)/platform/macos/KeySender.mm \
    $(SRC_DIR)/ActionHandler.cpp \
    $(SRC_DIR)/ResponseParser.cpp

# Targets
all: $(DYLIB)

# Rule to build the dynamic library
$(DYLIB): $(SRC)
	$(CC) $(CXXFLAGS) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FRAMEWORKS)

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
clean:
	rm -f $(DYLIB)


