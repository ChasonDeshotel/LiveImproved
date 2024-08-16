export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/InterceptKeys/libintercept_keys.dylib

killall Live

clang++ -dynamiclib -o libintercept_keys.dylib ./intercept_keys.mm -framework Cocoa

open /Applications/Ableton\ Live\ 11\ Suite.app
