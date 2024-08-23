killall Live

clang++ -std=c++17 -dynamiclib -o ./build/LiveImproved.dylib \
  ./Main.mm                            \
  ./lib/ApplicationManager.cpp         \
  ./lib/LogHandler.cpp                 \
  ./lib/platform/macos/GUISearchBox.mm \
  ./lib/platform/macos/IPC.cpp         \
  ./lib/platform/macos/PID.mm          \
  ./lib/platform/macos/EventHandler.mm \
  ./lib/platform/macos/KeySender.mm    \
  ./lib/ActionHandler.cpp              \
  -I./lib                              \
  -I./lib/platform/macos               \
  -framework Cocoa                     \
                                       \
	&& export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/LiveImproved/build/LiveImproved.dylib \
	&& open /Applications/Ableton\ Live\ 12\ Suite.app \
	; unset DYLD_INSERT_LIBRARIES
