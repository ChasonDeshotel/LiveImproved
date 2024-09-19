.PHONY: all clean

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
	@./build/LiveImproved_artefacts/LiveImproved.app/Contents/MacOS/LiveImproved

clean:
	@$(MAKE) -C build clean
