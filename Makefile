CC=gcc
CFLAGS=-Werror​ -Wall -pedantic

all:
	$(CC) sv.c -o server

clean:
	rm server