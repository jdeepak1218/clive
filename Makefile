CC=gcc
VERSION ?= 1.0.0
CFLAGS=-Wall -Wextra -pedantic -std=c99 -DCLIVE_VERSION=\"$(VERSION)\"

# Default build
clive: clive.c
	$(CC) $(CFLAGS) -o clive clive.c

# Platform-specific release builds
# Build these on the target platform (or use a cross-compiler):
#   Linux x86_64:  make clive-linux-x86_64
#   Linux ARM64:   make clive-linux-arm64
#   macOS ARM64:   make clive-macos-arm64
#   macOS x86_64:  make clive-macos-x86_64
clive-linux-x86_64: clive.c
	$(CC) $(CFLAGS) -o clive-linux-x86_64 clive.c

clive-linux-arm64: clive.c
	$(CC) $(CFLAGS) -o clive-linux-arm64 clive.c

clive-macos-arm64: clive.c
	$(CC) $(CFLAGS) -o clive-macos-arm64 clive.c

clive-macos-x86_64: clive.c
	$(CC) $(CFLAGS) -o clive-macos-x86_64 clive.c

# Build all platform binaries (build on respective target machines)
release: clive-linux-x86_64 clive-linux-arm64 clive-macos-arm64 clive-macos-x86_64

clean:
	rm -f clive clive-*

run: clive
	./clive test.txt

.PHONY: clean run release