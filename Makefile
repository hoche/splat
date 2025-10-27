all: build
.PHONY: all

build: ## compile on the local system
	@if [ ! -d ./build ]; then mkdir build; fi
	cd build && cmake --fresh .. && cmake --build . -j
.PHONY: build

clean: ## clean the local system
	if [ -d ./build ]; then rm -rf ./build; fi
.PHONY: clean

format: ## run clang-format
	clang-format -i src/*.cpp \
					src/*.h \
					utils/*.c \
					tests/*.cpp
.PHONY: format

test: ## run tests
	if [ -f ./build/splat_tests ]; then ./build/splat_tests; fi
.PHONY: test

# ==============================================================================
# Sanitizer Targets
# ==============================================================================

asan: ## run AddressSanitizer (detects memory errors)
	@cd build && cmake --build . -j --target asan
.PHONY: asan

ubsan: ## run UndefinedBehaviorSanitizer (detects undefined behavior)
	@cd build && cmake --build . -j --target ubsan
.PHONY: ubsan

tsan: ## run ThreadSanitizer (detects data races and deadlocks)
	@cd build && cmake --build . -j --target tsan
.PHONY: tsan

lsan: ## run LeakSanitizer (detects memory leaks)
	@cd build && cmake --build . -j --target lsan
.PHONY: lsan

msan: ## run MemorySanitizer (detects uninitialized reads, Clang only)
	@cd build && cmake --build . -j --target msan
.PHONY: msan

sanitizers: ## run all sanitizers
	@cd build && cmake --build . -j --target sanitizers
.PHONY: sanitizers

# ==============================================================================
# Memory Analysis Targets
# ==============================================================================

valgrind: ## run Valgrind memory checker (comprehensive)
	@cd build && cmake --build . -j --target valgrind
.PHONY: valgrind

valgrind-quick: ## run Valgrind memory checker (quick mode)
	@cd build && cmake --build . -j --target valgrind-quick
.PHONY: valgrind-quick

# ==============================================================================
# Static Analysis Targets
# ==============================================================================

clang-tidy: ## run Clang-Tidy static analysis
	@cd build && cmake --build . -j --target clang-tidy
.PHONY: clang-tidy

cppcheck: ## run CppCheck static analysis
	@cd build && cmake --build . -j --target cppcheck
.PHONY: cppcheck

cppcheck-verbose: ## run CppCheck static analysis (verbose)
	@cd build && cmake --build . -j --target cppcheck-verbose
.PHONY: cppcheck-verbose

analyze: ## run all static analysis tools
	@cd build && cmake --build . -j --target analyze
.PHONY: analyze

# ==============================================================================
# Combined Targets
# ==============================================================================

check-all: ## run all checks (sanitizers + analysis + valgrind)
	@cd build && cmake --build . -j --target check-all
.PHONY: check-all

check-quick: ## quick check (ASan + UBSan only)
	@cd build && cmake --build . -j --target asan
	@cd build && cmake --build . -j --target ubsan
.PHONY: check-quick

# ==============================================================================
# Help
# ==============================================================================

help: ## show this help message
	@echo "SPLAT! Build System"
	@echo "==================="
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-18s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "Sanitizers detect runtime errors:"
	@echo "  asan, ubsan, tsan, lsan, msan, sanitizers"
	@echo ""
	@echo "Static analysis finds code quality issues:"
	@echo "  clang-tidy, cppcheck, analyze"
	@echo ""
	@echo "Memory analysis:"
	@echo "  valgrind, valgrind-quick"
	@echo ""
	@echo "Combined checks:"
	@echo "  check-all, check-quick"
.PHONY: help
