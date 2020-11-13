#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3
mkdir $outputdir

for input in $inputdir/*
do
    for i in $(seq 1 $maxthreads)
    do
        echo Inputfile=$input NumThreads=$i
        #valgrind --tool=helgrind
        ./tecnicofs $input $input-$i.txt $i | tail -1
        #echo -ne "\n\n\n\n\n\n\n\n"
        mv $input-$i.txt $outputdir
    done
done
