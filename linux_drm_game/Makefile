CFLAGS=-std=c11 -Wall -O0 -g

LIBS=`pkg-config --cflags --libs libdrm libinput libudev`

all: run

drm_game: drm_game.c
	@bear -- gcc $(CFLAGS) -o $@ $< $(LIBS)

run: drm_game
	@./drm_game
