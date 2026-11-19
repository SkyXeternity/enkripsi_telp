CC := cc
OS := $(shell uname -s)
IS_WSL := $(shell if grep -qi microsoft /proc/version 2>/dev/null; then echo yes; else echo no; fi)

ifeq ($(OS),Darwin)
OPENSSL_PREFIX ?= $(shell brew --prefix openssl 2>/dev/null)
else ifeq ($(IS_WSL),yes)
OPENSSL_PREFIX ?= /usr
endif

CFLAGS := -Wall -Wextra -g
COMMON_INCLUDES := -Isrc -Ilibrary
OPENSSL_INCLUDES := -I$(OPENSSL_PREFIX)/include
OPENSSL_LIBS := -L$(OPENSSL_PREFIX)/lib -lcrypto -lpthread

.PHONY: all setup main main_input tests test_room test_packet crypto_test clean

# ============================================================
# Default
# ============================================================

all: main

# ============================================================
# Dependency setup (macOS or Windows/WSL only)
# Run: make -f makefile.mk setup
# ============================================================

setup:
ifeq ($(OS),Darwin)
	@echo "Menyiapkan dependency macOS..."
	@if ! xcode-select -p >/dev/null 2>&1; then \
		xcode-select --install; \
	fi
	@if ! command -v brew >/dev/null 2>&1; then \
		echo "Homebrew belum terpasang: https://brew.sh"; \
		exit 1; \
	fi
	brew install openssl
	@echo "Setup macOS selesai."
else ifeq ($(IS_WSL),yes)
	@echo "Menyiapkan dependency Windows/WSL..."
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		libssl-dev \
		pkg-config \
		valgrind \
		git
	@echo "Setup Windows/WSL selesai."
else
	@echo "Hanya macOS dan Windows/WSL yang didukung."
	@exit 1
endif

# ============================================================
# Application / server
# ============================================================

main: src/main.c src/server.c library/mongoose.c src/crypto.c src/crypto.h src/room.c src/room.h
	$(CC) $(CFLAGS) \
		src/main.c \
		library/mongoose.c \
		src/crypto.c \
		src/room.c \
		src/server.c \
		-o main \
		$(COMMON_INCLUDES) \
		$(OPENSSL_INCLUDES) \
		$(OPENSSL_LIBS)

# ============================================================
# Manual input test from src/main.c (room module only)
# Run: make -f makefile.mk main_input && ./build/main_input
# ============================================================

main_input: src/main.c src/room.c src/room.h
	mkdir -p build
	$(CC) $(CFLAGS) \
		src/main.c \
		src/room.c \
		-o build/main_input \
		$(COMMON_INCLUDES)

# ============================================================
# Unit tests
# ============================================================

tests: test_room test_packet crypto_test

test_room: tests/test_room.c src/room.c src/room.h
	$(CC) $(CFLAGS) \
		tests/test_room.c \
		src/room.c \
		-o test_room \
		$(COMMON_INCLUDES)

test_packet: tests/test_packet.c src/packet.c src/packet.h src/crypto.h
	$(CC) $(CFLAGS) \
		tests/test_packet.c \
		src/packet.c \
		-o test_packet \
		$(COMMON_INCLUDES)

crypto_test: tests/test_crypto.c src/crypto.c src/crypto.h
	$(CC) $(CFLAGS) \
		tests/test_crypto.c \
		src/crypto.c \
		-o crypto_test \
		$(COMMON_INCLUDES) \
		$(OPENSSL_INCLUDES) \
		$(OPENSSL_LIBS)
		
crypto_packet_test: tests/test_crypto_packet.c \
	src/crypto.c src/crypto.h \
	src/packet.c src/packet.h
	$(CC) $(CFLAGS) \
		tests/test_crypto_packet.c \
		src/crypto.c \
		src/packet.c \
		-o crypto_packet_test \
		$(COMMON_INCLUDES) \
		$(OPENSSL_INCLUDES) \
		$(OPENSSL_LIBS)

# ============================================================
# Cleanup (only runs when explicitly requested)
# ============================================================

clean:
	rm -f main test_room test_packet crypto_test
	rm -f build/main_input

# ============================================================
# Previous standalone crypto command (kept as reference)
# ============================================================

# crypto: src/crypto.c
# 	$(CC) src/crypto.c -o crypto \
# 		$(COMMON_INCLUDES) $(OPENSSL_INCLUDES) $(OPENSSL_LIBS)
