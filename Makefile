CC_LINUX   = gcc
CC_WINDOWS = x86_64-w64-mingw32-gcc
CFLAGS     = -Wall -Wextra -O2
PREFIX     = /usr/local/bin

SRCS       = main.c jsonfylinx.c
LIB_SRC    = jsonfylinx.c
HEADER     = jsonfylinx.h

BIN_LINUX   = bin/jsonfylinx
BIN_WINDOWS = bin/jsonfylinx.exe
LIB_STATIC  = bin/libjsonfylinx.a
LIB_SHARED  = bin/libjsonfylinx.so

.PHONY: all linux windows lib release install uninstall clean

all: linux

linux: $(BIN_LINUX)

windows: $(BIN_WINDOWS)

lib: $(LIB_STATIC) $(LIB_SHARED)

release: linux windows lib

$(BIN_LINUX): $(SRCS) $(HEADER) | bin
	$(CC_LINUX) $(CFLAGS) -o $@ $(SRCS)

$(BIN_WINDOWS): $(SRCS) $(HEADER) | bin
	$(CC_WINDOWS) $(CFLAGS) -o $@ $(SRCS)

# Static library — primary artifact for linking from another program (e.g. Rust FFI).
$(LIB_STATIC): $(LIB_SRC) $(HEADER) | bin
	$(CC_LINUX) $(CFLAGS) -c $(LIB_SRC) -o bin/jsonfylinx.o
	ar rcs $@ bin/jsonfylinx.o

# Shared library — for runtime/dynamic linking.
$(LIB_SHARED): $(LIB_SRC) $(HEADER) | bin
	$(CC_LINUX) $(CFLAGS) -fPIC -shared -o $@ $(LIB_SRC)

bin:
	mkdir -p bin

install: linux
	install -m 755 $(BIN_LINUX) $(PREFIX)/jsonfylinx

uninstall:
	rm -f $(PREFIX)/jsonfylinx

clean:
	rm -rf bin/
