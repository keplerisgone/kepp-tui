CMAKE ?= cmake
BUILD_DIR ?= build
BUILD_TYPE ?= Debug
PREFIX ?= /usr/local

.PHONY: all configure build test release install clean

all: build

configure:
	@command -v $(CMAKE) >/dev/null 2>&1 || { \
		echo "error: cmake is required (macOS: brew install cmake, Debian/Ubuntu: sudo apt install cmake build-essential)" >&2; \
		exit 1; \
	}
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

test: build
	$(CMAKE) --build $(BUILD_DIR) --target test

release:
	$(MAKE) build BUILD_DIR=build-release BUILD_TYPE=Release

install: release
	$(CMAKE) --install build-release --prefix $(PREFIX)

clean:
	$(CMAKE) -E rm -rf build build-release
