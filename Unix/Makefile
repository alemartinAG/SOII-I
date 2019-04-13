CC=gcc
CFLAGS=-Werror​ -Wall -pedantic

all:
	$(CC) server_cc.c -o server
	$(CC) client_cc.c -o client

runserver:
	./server control

runclient:
	./client control

cppcheck:
	cppcheck --enable=all  --suppress=missingIncludeSystem client_cc.c server_cc.c 2>err.txt

clean:
	rm server
	rm client
	rm control
	rm ImgSat.jpg