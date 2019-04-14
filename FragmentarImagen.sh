#!/bin/bash

#Script necesario para poder utilizar funcion start scanning
echo "Fragmentando Imagenes (puede tomar unos segundos)"
base64 -w 0 ImagenSatelital.jpg > ImgBase64
cp ImgBase64 Internet/Image/ImgBase64
cp ImgBase64 Unix/Image/ImgBase64
cp ImgBase64 Raspberry/Client/Image/ImgBase64
rm ImgBase64
cd Internet/Image
split -a 6 -b 1448 -d ImgBase64
rm ImgBase64
cd ../
cd ../
cd Unix/Image
split -a 6 -b 1448 -d ImgBase64
rm ImgBase64
cd ../
cd ../
cd Raspberry/Client/Image
split -a 6 -b 1448 -d ImgBase64
rm ImgBase64