CFLAGS=-std=c11 -pedantic -Wall -O0 -g

LIBS=$(shell pkg-config --cflags --libs libinput libudev)

all: run

build: input.c
	@gcc $(CFLAGS) -o input $< $(LIBS)

run: build
	@sudo ./input

