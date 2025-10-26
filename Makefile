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
