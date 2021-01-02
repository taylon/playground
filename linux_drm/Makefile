CFLAGS=-std=c11 -Wall -O0 -g

LIBS=`pkg-config --cflags --libs libdrm libkms cairo`

all: run

modeset: modeset.c
	@bear -- gcc $(CFLAGS) -o modeset $< $(LIBS)
	@./$@

atomic: modeset-atomic.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)
	@./$@

double: double_buffering.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)
	@./$@

page_flip: page_flip.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)
	@./$@

drawing: drawing.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)

run: drawing
	@./drawing
