#!/bin/bash

ps -p $(pidof server) --format %cpu --format %mem > cpumem.txt