#!/bin/bash

base64 -w 0 ImagenTierra.jpg > ImgBase64
split -a 6 -b 1448 -d ImgBase64