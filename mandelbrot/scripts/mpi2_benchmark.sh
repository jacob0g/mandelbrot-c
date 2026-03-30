#!/bin/bash --login

#SBATCH --account=courses0100
#SBATCH --array=0-9  # must be 0-(N_POINTS-1)
#SBATCH --nodes=1
#SBATCH --ntasks=10
#SBATCH --cpus-per-task=1
#SBATCH --time=1:0

N_POINTS=10
MIN_CHUNK=10
MAX_CHUNK=100000000  # 10^8

chunksizes=($(awk -v n="$N_POINTS" -v min="$MIN_CHUNK" -v max="$MAX_CHUNK" \
    'BEGIN { for (i=0; i<n; i++) printf "%d\n", min * exp(log(max/min) * i / (n-1)) }'))

echo "[$SLURM_ARRAY_TASK_ID] ${chunksizes[$SLURM_ARRAY_TASK_ID]}"
srun -n 10 build/mpi_2 ${chunksizes[$SLURM_ARRAY_TASK_ID]}
