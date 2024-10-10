# default target, configure and build CLI
all: configure build

CLI_BUILD_DIR = ./build/macos-cli
XCODE_BUILD_DIR = ./build/xcode

configure: # defaults to cli
	@mkdir -p $(CLI_BUILD_DIR)
	@cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B $(CLI_BUILD_DIR)
	@ln -sf $(realpath $(CLI_BUILD_DIR))/compile_commands.json ./build/compile_commands.json

build: configure
	@cmake --build $(CLI_BUILD_DIR) --target LiveImproved -- VERBOSE=1

test:
	@$(MAKE) -C test

run-tests:
	@cd build && ctest --output-on-failure

run: build
	@./build/macos-cli/LiveImproved_artefacts/Debug/LiveImproved.app/Contents/MacOS/LiveImproved

clean:
	@$(MAKE) -C ./build/macos-cli clean
	@$(MAKE) -C ./build/xcode clean
	@rm -f ./build/compile_commands.json

tidy:
	@$(MAKE) -C ./build/macos-cli tidy
