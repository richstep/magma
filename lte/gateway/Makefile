.PHONY: all build clean help log logs run status test

GATEWAY_C_DIR = $(MAGMA_ROOT)/lte/gateway/c
GRPC_CPP_PLUGIN_PATH ?= `which grpc_cpp_plugin`
BUILD_TYPE ?= Debug

OAI_FLAGS = -DEMBEDDED_SGW=True -DENABLE_OPENFLOW=True
TEST_FLAG = -DBUILD_TESTS=1

all: build

build: build_python build_common build_oai build_sctpd build_session_manager ## Build all

test: test_python test_oai test_session_manager ## Run all tests

clean: clean_python  ## Clean all builds
	rm -rf $(C_BUILD)

clean_python: ## Clean Python-only builds
	make -C $(MAGMA_ROOT)/lte/gateway/python clean

start: ## Start all services
	sudo service magma@magmad start

stop: ## Stop all services
	sudo service magma@* stop

restart: stop start ## Restart all services

run: build restart ## Build and run all services

status: ## Status of all services
	sudo service magma@* status

log: ## Follow logs for magmad service
	sudo journalctl -fu magma@magmad | egrep 'error|$$' -i --color

logs: ## Follow logs for all services
	sudo journalctl -fu magma@* | egrep 'error|$$' -i --color

# Ref: https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
help: ## Show documented commands
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-25s\033[0m %s\n", $$1, $$2}'


# run_cmake BUILD_DIRECTORY, FILE_DIRECTORY, FLAGS, ENV
define run_cmake
mkdir -p $(1)
cd $(1) && $(4) cmake $(2) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(3) -GNinja
ninja -C $(1)
endef

# run_scanbuild BUILD_DIRECTORY, FILE_DIRECTORY, FLAGS
define run_scanbuild
$(eval REPORT_DIR = "$(1)/reports")
mkdir -p $(1)
mkdir -p $(REPORT_DIR)
cd $(1) && scan-build cmake $(2) -DCMAKE_BUILD_TYPE=Debug $(3) -GNinja
scan-build -o $(REPORT_DIR) ninja -C $(1)
cp -r $(REPORT_DIR) $(MAGMA_ROOT)
@echo "Reports in magma/reports/.../index.html"
endef

# run_ctest BUILD_DIRECTORY, FILE_DIRECTORY, FLAGS
define run_ctest
$(call run_cmake, $(1), $(2), $(3) $(TEST_FLAG))
cd $(1) && ctest --output-on-failure
endef

build_python: stop ## Build Python environment
	make -C $(MAGMA_ROOT)/lte/gateway/python buildenv

build_common: ## Build shared libraries
	$(call run_cmake, $(C_BUILD)/magma_common, $(MAGMA_ROOT)/orc8r/gateway/c/common, )

build_oai: build_common ## Build OAI
	$(call run_cmake, $(C_BUILD)/oai, $(GATEWAY_C_DIR)/oai, $(OAI_FLAGS))

build_sctpd: build_common ## Build SCTPD
	$(call run_cmake, $(C_BUILD)/sctpd, $(GATEWAY_C_DIR)/sctpd, )

build_session_manager: build_common ## Build session manager
	$(call run_cmake, $(C_BUILD)/session_manager, $(GATEWAY_C_DIR)/session_manager, )

# Catch all for c services that don't have custom flags
# This works with build_dpi
build_%:
	$(call run_cmake, $(C_BUILD)/$*, $(MAGMA_ROOT)/c/$*, )

scan_oai: ## Scan OAI
	$(call run_scanbuild, $(C_BUILD)/scan/oai, $(GATEWAY_C_DIR)/oai, $(OAI_FLAGS))

format_oai: ## Format OAI
	find $(GATEWAY_C_DIR)/oai \( -iname "*.c" -o -iname "*.cpp" -o -iname "*.h" \) -exec \
	clang-format --style=file -i {} \;

test_python: stop ## Run all Python-specific tests
	make -C $(MAGMA_ROOT)/lte/gateway/python test_all

test_oai: build_common ## Run all OAI-specific tests
	$(call run_ctest, $(C_BUILD)/oai, $(GATEWAY_C_DIR)/oai, $(OAI_FLAGS))

# Catch all for c service tests
# This works with test_dpi and test_session_manager
test_%: stop build_common
	$(call run_ctest, $(C_BUILD)/$*, $(GATEWAY_C_DIR)/$*, )

# TODO: include coverage of dpim when it is included in make build
COV_OUTPUT_OAI = $(OAI_BUILD)/coverage.info
COV_OUTPUT_SM = $(C_BUILD)/session_manager/coverage.info
COV_OUTPUT_TOTAL = $(C_BUILD)/coverage.info
# Put HTML within magma directory, so it can be accessed from outside VM
COV_HTML_DIR_OAI = $(GATEWAY_C_DIR)/oai/code_coverage
COV_HTML_DIR_SM = $(GATEWAY_C_DIR)/session_manager/code_coverage
COV_HTML_DIR_TOTAL = $(MAGMA_ROOT)/c/code_coverage

SLEEP_SECS = 10

coverage: ## Generate full code coverage report
	# Remove any previous gcov output files
	rm -f `find $(OAI_BUILD) -name *.gcda`
	rm -f `find $(C_BUILD)/session_manager -name *.gcda`
	# Stop processes to generate gcda
	sudo service magma@mme stop
	sudo pkill -INT sessiond
	# Wait for gcov output to be generated
	@echo "Waiting for $(SLEEP_SECS) for gcov to write files"
	sleep $(SLEEP_SECS)
	# Capture coverage info under different directories individually
	lcov --capture --directory $(OAI_BUILD) --output-file $(COV_OUTPUT_OAI)
	lcov --capture --directory $(C_BUILD)/session_manager --output-file $(COV_OUTPUT_SM)
	# Merge coverage info
	lcov --add-tracefile $(COV_OUTPUT_OAI) --add-tracefile $(COV_OUTPUT_SM) --output-file $(COV_OUTPUT_TOTAL)
	# Remove coverage info regarding libraries from /usr/include/
	lcov --remove $(COV_OUTPUT_TOTAL) '/usr/include/*' '/usr/local/include/*' -o $(COV_OUTPUT_TOTAL) --quiet
	# Generate html which shows coverage with graph
	genhtml $(COV_OUTPUT_TOTAL) --output-directory $(COV_HTML_DIR_TOTAL)
	@echo "Generated coverage output to $(COV_HTML_DIR_TOTAL)/index.html"

coverage_oai: ## Generate code coverage report for OAI
	rm -f `find $(OAI_BUILD) -name *.gcda`
	sudo service magma@mme stop
	@echo "Waiting for $(SLEEP_SECS) for gcov to write files"
	sleep $(SLEEP_SECS)
	lcov --capture --directory $(OAI_BUILD) --output-file $(COV_OUTPUT_OAI)
	lcov --remove $(COV_OUTPUT_OAI) '/usr/include/*' '/usr/local/include/*' -o $(COV_OUTPUT_OAI) --quiet
	genhtml $(COV_OUTPUT_OAI) --output-directory $(COV_HTML_DIR_OAI)
	@echo "Generated coverage output to $(COV_HTML_DIR_OAI)/index.html"

coverage_sm: ## Generate code coverage report for session manager
	rm -f `find $(C_BUILD)/session_manager -name *.gcda`
	sudo pkill -INT sessiond
	sleep $(SLEEP_SECS)
	lcov --capture --directory $(C_BUILD)/session_manager --output-file $(COV_OUTPUT_SM)
	lcov --remove $(COV_OUTPUT_SM) '/usr/include/*' '/usr/local/include/*' -o $(COV_OUTPUT_SM) --quiet
	genhtml $(COV_OUTPUT_SM) --output-directory $(COV_HTML_DIR_SM)
	@echo "Generated coverage output to $(COV_HTML_DIR_SM)/index.html"

code_stats: ## Generate lines-of-code statistics for magma project
	sudo apt-get install -y cloc
	cloc .
