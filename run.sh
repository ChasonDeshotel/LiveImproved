
killall Live

#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/read
#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/write

clang++ -std=c++17 -dynamiclib -o ./build/LiveImproved.dylib \
  ./Main.mm                            \
  ./lib/ApplicationManager.cpp         \
  ./lib/LogHandler.cpp                 \
  ./lib/platform/macos/EventHandler.mm \
  ./lib/platform/macos/KeySender.mm    \
  -I./lib                              \
  -I./lib/platform/macos               \
  -framework Cocoa                     \
                                       \
	&& export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/LiveImproved/build/LiveImproved.dylib \
	&& open /Applications/Ableton\ Live\ 12\ Suite.app \
	; unset DYLD_INSERT_LIBRARIES

#  ./lib/ActionHandler.cpp              \
#  ./lib/platform/macos/GUI.mm          \
#  ./lib/platform/macos/IPCManager.cpp  \
#  ./lib/platform/macos/Init.mm         \
