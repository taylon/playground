CFLAGS=-std=c11 -Wall -O0 -g

LIBS=`pkg-config --cflags --libs libdrm libkms cairo`

all: run

modeset: modeset.c
	@bear -- gcc $(CFLAGS) -o modeset $< $(LIBS)
	@./$@

page_flip: page_flip.c
	@bear -- gcc $(CFLAGS) -o page_flip $< $(LIBS)
	@./$@

drawing: drawing.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)

run: drawing
	@./drawing
