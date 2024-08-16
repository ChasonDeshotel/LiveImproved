
killall Live

clang++ -dynamiclib -o libintercept_keys.dylib ./Main.mm ./CustomAlert.mm -framework Cocoa \
	&& export DYLD_INSERT_LIBRARIES=/Users/cdeshotel/Scripts/Ableton/InterceptKeys/libintercept_keys.dylib \
	&& open /Applications/Ableton\ Live\ 11\ Suite.app \
	; unset DYLD_INSERT_LIBRARIES
