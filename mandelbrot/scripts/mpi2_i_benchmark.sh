#!/bin/bash --login

#SBATCH --account=courses0100
#SBATCH --nodes=1
#SBATCH --ntasks=10
#SBATCH --cpus-per-task=1
#SBATCH --time=1:0

echo "Benchmarking: mpi2_i"
srun -n 10 build/mpi_2_i 5000
