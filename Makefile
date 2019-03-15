CC=gcc
CFLAGS=-Wall -pedantic

all:
	$(CC) sv.c -o server

clean:
	rm server