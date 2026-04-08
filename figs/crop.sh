#!/bin/bash
name=$1
fname="${name##*/}"
extn=".${fname##*.}"
name="${fname%.*}"

if [[ "$extn" = ".pdf" ]];
then
    pdfcrop --margin 0 $fname && mv $name-crop$extn $fname
else
    echo "ERROR: argument '$fname' is not a PDF"
fi
exit
