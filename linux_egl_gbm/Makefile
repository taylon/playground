CFLAGS=-std=c11 -Wall -O0 -g

all: run

egl: egl.c shaders.c
	@bear -- gcc $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs libdrm gbm egl gl`

run: egl
	@./egl

