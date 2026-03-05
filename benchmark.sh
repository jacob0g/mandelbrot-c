#!/bin/sh
MYSCRATCH="/scratch/courses0100/jgallop"

./deploy.sh

ssh setonix "cd $MYSCRATCH/mandelbrot && \
    make clean && \
    make BENCHMARK=1 && \
    srun -n 10 --time 0:5 build/mpi_1"

rsync -a data-mover:$MYSCRATCH/mandelbrot/benchmarks/*.dat $PWD/benchmarks/
