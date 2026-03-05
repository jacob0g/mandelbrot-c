#!/bin/sh
MYSCRATCH="/scratch/courses0100/jgallop"

rsync -a ./mandelbrot/ data-mover:$MYSCRATCH/mandelbrot/

if [ "$1" = "--now" ]; then
    ssh -t setonix "cd $MYSCRATCH/mandelbrot ; bash --login"
fi
