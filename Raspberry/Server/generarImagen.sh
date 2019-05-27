#!/bin/bash

cat Recibido/x{000000..050000} > Recibido/pt01 2>/dev/null
cat Recibido/x{050001..100000} > Recibido/pt02 2>/dev/null
cat Recibido/pt* > Recibido/imagenB64
base64 -d Recibido/imagenB64 > ImgSat.jpg
sleep 5
rm -r Recibido
mkdir Recibido