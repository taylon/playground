.PHONY: clean run debug

CC=gcc
CFLAGS=-std=c11 -pedantic -Wall -O0 -g -DWLR_USE_UNSTABLE 

LIBS=\
	 $(shell pkg-config --cflags --libs wlroots) \
	 $(shell pkg-config --cflags --libs pixman-1) \
	 $(shell pkg-config --cflags --libs wayland-server) \
	 $(shell pkg-config --cflags --libs xkbcommon) 
	 # $(shell pkg-config --cflags --libs wayland-client) \
	 # $(shell pkg-config --cflags --libs wayland-protocols) \
	 # $(shell pkg-config --cflags --libs wayland-egl) \
	 # $(shell pkg-config --cflags --libs egl) \
	 # $(shell pkg-config --cflags --libs libinput) \
	 # $(shell pkg-config --cflags --libs libdrm) \
	 # $(shell pkg-config --cflags --libs gbm) \
	 # $(shell pkg-config --cflags --libs xcb) \
	 # $(shell pkg-config --cflags --libs glesv2) 
	 

WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)
WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_PROTOCOLS_SOURCES=xdg-shell-protocol.h xdg-shell-protocol.c

SOURCES=src/compositor.c
BIN_NAME=compositor

all: build run

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

xdg-shell-protocol.c: xdg-shell-protocol.h
	$(WAYLAND_SCANNER) private-code \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

build: $(SOURCES) $(WAYLAND_PROTOCOLS_SOURCES)
	@bear -- $(CC) $(CFLAGS) -I. -o $(BIN_NAME) $(SOURCES) $(WAYLAND_PROTOCOLS_SOURCES) $(LIBS)

debug: build
	@gdb $(BIN_NAME)

clean:
	@rm -f $(BIN_NAME) $(WAYLAND_PROTOCOLS_SOURCES)

run: 
	@./$(BIN_NAME)
	
