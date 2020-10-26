#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3
usage="Usage: ./runTests.sh <inputdir> <outputdir> numthreads"

if test ! -f "tecnicofs"; then
    echo "Please run make to create executable"
    exit 1
fi

if test $# -ne 3; then
    echo $usage
    exit 1
fi

if test ! -d $inputdir; then
   echo "Make sure your input directory exists"
   exit 1
fi

if test $maxthreads -lt 1; then
    echo "Invalid number of threads, must be greater than 0"
    exit 1
fi

if test ! -d $outputdir; then
    mkdir $outputdir
fi

for input in $inputdir/*
do
    inputfile=$(echo $input | rev | cut -d / -f1 | rev)
    for i in $(seq 1 $maxthreads)
    do
        echo Inputfile=$inputfile NumThreads=$i
        ./tecnicofs $input $outputdir/$(echo $inputfile | cut -d . -f1)-$i.txt $i | tail -1
    done
done
