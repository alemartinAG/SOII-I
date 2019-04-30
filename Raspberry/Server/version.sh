#!/bin/bash

gcc client_cc.c -o update
base64 -w 0 update > update64