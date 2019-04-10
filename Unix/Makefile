CC=gcc
CFLAGS=-Werror​ -Wall -pedantic

all:
	$(CC) server_cc.c -o server
	$(CC) client_cc.c -o client

runserver:
	./server control

runclient:
	./client control

clean:
	rm server
	rm client
	rm control
	rm client_u