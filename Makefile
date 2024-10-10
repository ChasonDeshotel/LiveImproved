.PHONY: all clean tidy

# clang-tidy -checks='cppcoreguidelines-*,modernize-*' <file.cpp>
#
# standard build
# make -C build VERBOSE=1 LDFLAGS="-v"
# cmake -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

# xcode build
# cmake -G "Xcode" -DBUILD_TESTING=OFF -B./build-xcode .

#.PHONY: bear-build

#compile_commands:
#	@bear -- make clean
#	@bear -- make all

#cmake-configure:
#	@mkdir -p build
#	@cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B ./build

all:
	@$(MAKE) -C build

test:
	@$(MAKE) -C test

run-tests:
	@cd build && ctest --output-on-failure

run:
	@./build/LiveImproved_artefacts/Debug/LiveImproved.app/Contents/MacOS/LiveImproved

clean:
	@$(MAKE) -C build clean

tidy:
	@$(MAKE) -C build tidy
