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


