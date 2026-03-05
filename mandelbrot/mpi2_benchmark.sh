#!/bin/bash --login

#SBATCH --account=courses0100
#SBATCH --array=0-9
#SBATCH --nodes=1
#SBATCH --ntasks=10
#SBATCH --cpus-per-task=1
#SBATCH --time=00:30

chunksizes=(100 500 1000 2000 5000 10000 50000 100000 250000 500000)

srun -n 10 build/mpi_2 ${chunksizes[$SLURM_ARRAY_TASK_ID]}
