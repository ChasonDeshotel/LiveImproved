
killall Live

#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/read
#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/write

clang++ -std=c++17 -dynamiclib -o libintercept_keys.dylib \
  ./lib/ActionHandler.cpp              \
  ./lib/ApplicationManager.cpp         \
  ./lib/Log.cpp                        \
  ./lib/platform/macos/EventHandler.mm \
  ./lib/platform/macos/GUI.mm          \
  ./lib/platform/macos/IPCUtils.cpp    \
  ./lib/platform/macos/Init.mm         \
  ./lib/platform/macos/KeySender.mm    \
  -I./lib                              \
  -I./lib/platform/macos               \
  -framework Cocoa                     \
                                       \
	&& export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/InterceptKeys/libintercept_keys.dylib \
	&& open /Applications/Ableton\ Live\ 12\ Suite.app \
	; unset DYLD_INSERT_LIBRARIES
