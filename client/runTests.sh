#!/bin/bash

inputdir=$1
sock="/tmp/abc.sock"

if test ! -f "tecnicofs-client"; then
    echo "Please run make to create executable"
    exit 1
fi

for input in $inputdir/*
do
    inputfile=$(echo $input | rev | cut -d / -f1 | rev)
    for i in $(seq 1 $maxthreads)
    do
        echo Inputfile=$inputfile NumThreads=$i
        ./tecnicofs-client $input $sock #| tail -1
    done
done
