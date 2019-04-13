CC=gcc
CFLAGS=-Werror​ -Wall -pedantic

all:
	$(CC) server_cc.c -o server
	$(CC) client_cc.c -o client

runserver:
	./server 27415

runclient:
	./client localhost 27415

clean:
	rm server
	rm client
	rm ImgSat.jpg