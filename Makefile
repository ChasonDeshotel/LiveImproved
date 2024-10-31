# default target, configure and build CLI
all: configure build

CLI_BUILD_DIR = ./build/macos-cli
XCODE_BUILD_DIR = ./build/xcode

configure: # defaults to cli
	@mkdir -p $(CLI_BUILD_DIR)
	@cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B $(CLI_BUILD_DIR)
	@ln -sf $(realpath $(CLI_BUILD_DIR))/compile_commands.json ./build/compile_commands.json

configure-tests:
	@mkdir -p $(CLI_BUILD_DIR)
	@cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTS=ON -S . -B $(CLI_BUILD_DIR)
	@ln -sf $(realpath $(CLI_BUILD_DIR))/compile_commands.json ./build/compile_commands.json

configure-xcode:
	@mkdir -p $(XCODE_BUILD_DIR)
	@cmake -G "Xcode" -S . -B $(XCODE_BUILD_DIR)

build: configure
	@cmake --build $(CLI_BUILD_DIR) --target LiveImproved -- VERBOSE=1

xcode-build: configure-xcode
	@xcodebuild -project $(XCODE_BUILD_DIR)/LiveImproved.xcodeproj -scheme LiveImproved build -verbose

test:
	@$(MAKE) -C test

run_tests: configure-tests
	@if [ -z "$(TEST)" ]; then \
		echo "Building and running all tests..."; \
		cmake --build $(CLI_BUILD_DIR) --target build_tests && \
		cd $(CLI_BUILD_DIR) && ctest --output-on-failure -V; \
	else \
		echo "Building and running test: $(TEST)"; \
		cmake --build $(CLI_BUILD_DIR) --target test_system_test_$(TEST) && \
		cd $(CLI_BUILD_DIR) && ./bin/test_system_test_$(TEST) --test-case="$(TEST) basic functionality" -s; \
	fi

run: configure build
	@./build/macos-cli/LiveImprovedMain

clean:
	@rm -rf $(CLI_BUILD_DIR)
	@rm -rf $(XCODE_BUILD_DIR)
	@rm -f ./build/compile_commands.json

tidy:
	@$(MAKE) -C ./build/macos-cli tidy

tidy-file:
	@if [ -z "$(SOURCE_FILE)" ]; then \
		echo "Please specify a SOURCE_FILE to run clang-tidy on."; \
		echo "Usage: make tidy-file SOURCE_FILE=path/to/your/file.cpp"; \
		exit 1; \
	fi
	@CLANG_TIDY_EXE=$$(which clang-tidy) && \
	BUILD_DIR=$(CLI_BUILD_DIR) && \
	"$${CLANG_TIDY_EXE}" \
		"-p=$${BUILD_DIR}" \
		"--header-filter=.*(?<!lib/).*" \
		"--checks=-*,cppcoreguidelines-*,modernize-*" \
		"--warnings-as-errors=*" \
		"$(SOURCE_FILE)"
