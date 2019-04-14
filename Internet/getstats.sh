#!/bin/bash

ps -p $(pidof client) --format %cpu --format %mem > cpumem.txt
