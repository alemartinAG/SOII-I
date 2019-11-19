CC=gcc
CFLAGS=-Werror -Wall -pedantic

all:
	$(CC) $(CFLAGS) client_cc.c -o client

runclient:
	./client 10.0.0.10 27415

runwifi:
	./client 192.168.0.133 27415

clean:
	rm client
