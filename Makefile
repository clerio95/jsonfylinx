CC_LINUX   = gcc
CC_WINDOWS = x86_64-w64-mingw32-gcc
CFLAGS     = -Wall -Wextra -O2
SRC        = main.c
PREFIX     = /usr/local/bin

BIN_LINUX   = bin/jsonfylinx
BIN_WINDOWS = bin/jsonfylinx.exe

.PHONY: all linux windows release install uninstall clean

all: linux

linux: $(BIN_LINUX)

windows: $(BIN_WINDOWS)

release: linux windows

$(BIN_LINUX): $(SRC) | bin
	$(CC_LINUX) $(CFLAGS) -o $@ $(SRC)

$(BIN_WINDOWS): $(SRC) | bin
	$(CC_WINDOWS) $(CFLAGS) -o $@ $(SRC)

bin:
	mkdir -p bin

install: linux
	install -m 755 $(BIN_LINUX) $(PREFIX)/jsonfylinx

uninstall:
	rm -f $(PREFIX)/jsonfylinx

clean:
	rm -rf bin/
