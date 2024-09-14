.PHONY: all clean

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
