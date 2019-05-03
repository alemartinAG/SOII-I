#!/bin/bash

kill -9 $(pidof client)
cat UPDATE/cl* > client_update.c
rm -r UPDATE
mkdir UPDATE
rm client
gcc client_update.c -o client
rm client_update.c
chmod +x client
./client 192.168.0.133 27415