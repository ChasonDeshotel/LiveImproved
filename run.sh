
killall Live

#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/read
#mkfifo /Users/cdeshotel/Scripts/Ableton/InterceptKeys/write

clang++ -dynamiclib -o libintercept_keys.dylib ./Main.mm ./CustomAlert.mm -framework Cocoa \
	&& export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/InterceptKeys/libintercept_keys.dylib \
	&& open /Applications/Ableton\ Live\ 12\ Suite.app \
	; unset DYLD_INSERT_LIBRARIES
