#!/bin/sh
MYSCRATCH="/scratch/courses0100/jgallop"

rsync -a ./mandelbrot/ data-mover:$MYSCRATCH/mandelbrot/
