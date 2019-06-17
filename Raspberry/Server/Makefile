CC=gcc
CFLAGS=-Werror -Wall -pedantic

all:
	$(CC) $(CFLAGS) server_cc.c -o server

runserver:
	./server 27415

clean:
	rm server
	rm ImgSat.jpg