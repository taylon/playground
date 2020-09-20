all: server client

CFLAGS = -std=c11 -pedantic -Wall -Os
CC = gcc

server: server.c
	${CC} -o $@ ${CFLAGS} server.c

client: client.c
	${CC} -o $@ ${CFLAGS} client.c
