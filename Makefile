.PHONY: all clean

all:
	@$(MAKE) -C build

test:
	@$(MAKE) -C test

run-tests:
	@cd build && ctest --output-on-failure

clean:
	@$(MAKE) -C build clean
