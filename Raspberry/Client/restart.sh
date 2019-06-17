#!/bin/bash

kill -9  $(pidof client)
rm client
#gcc client_update.c -o client
#rm client_update.c
mv update client
chmod +x client
./client 192.168.0.133 27415
