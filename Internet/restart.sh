#!/bin/bash

clientpid=$ pidof client
killall client
mv client_u client
chmod +x client
./client localhost 27415