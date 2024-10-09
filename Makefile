.PHONY: all clean tidy

# clang-tidy -checks='cppcoreguidelines-*,modernize-*' <file.cpp>
#
# standard build
# cmake -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

# xcode build
# cmake -G "Xcode" -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DBUILD_TESTING=OFF ..

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
