#!/bin/sh
MYSCRATCH="/scratch/courses0100/jgallop"
BENCHMARK_DIR="$PWD/data/benchmarks"
TARGET_BENCHMARK_DIR="$MYSCRATCH/mandelbrot/benchmarks"

./deploy.sh

ssh setonix "cd $MYSCRATCH/mandelbrot && \
    make clean && \
    make BENCHMARK=1 && \
    srun -n 10 --time 0:5 build/mpi_1"

rsync -a data-mover:$TARGET_BENCHMARK_DIR/ $BENCHMARK_DIR/
