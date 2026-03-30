#!/bin/sh
MYSCRATCH="/scratch/courses0100/jgallop"
TARGET_DIR="$MYSCRATCH/mandelbrot"
TARGET_BENCHMARK_DIR="$TARGET_DIR/benchmarks"
LOCAL_BENCHMARK_DIR="$PWD/data/benchmarks"

# Runtime flags
COPY_RESULTS=false
TASKS=()
N="10" # Number of processes
GRID_WIDTH="15000"

help() {
    echo "Usage: $0 [--copy-results] [--tasks [1][,2][,3]] [-n 10] [--grid-width 15000]"
}

# Arg Parsing
while [[ $# -gt 0 ]]; do
    case "$1" in
        --copy-results)
            COPY_RESULTS=true
            shift
            ;;
        --grid-width)
            GRID_WIDTH="$2"
            shift 2
            ;;
        --tasks)
            # comma-separated values: `--tasks 1,2` -> (1,2)
            IFS=',' read -ra TASKS <<< "$2"
            shift 2
            ;;
        -n)
            N="$2"
            shift 2
            ;;
        --help)
            help
            exit 0
            ;;
        *)
            echo "Invalid Argument: $1" >&2
            help
            exit 1
            ;;
    esac
done

if [[ ${#TASKS[@]} -gt 0 ]]; then
    # Build SSH command
    SSH_CMD="cd $TARGET_DIR && make clean && make BENCHMARK=1 WIDTH=$GRID_WIDTH"
    
    for task in "${TASKS[@]}"; do
        SSH_CMD+=" && mkdir -p $TARGET_BENCHMARK_DIR/task_$task"
        SSH_CMD+=" && sbatch scripts/mpi${task}_benchmark.sh"
        # SSH_CMD+=" && srun -n $N --time 1:0 build/mpi_$task"
    done

    ./deploy.sh
    echo "> $SSH_CMD"
    ssh setonix "$SSH_CMD"
    COPY_RESULTS=true
fi

# Copy results
if $COPY_RESULTS; then
    rsync -a data-mover:$TARGET_BENCHMARK_DIR/ $LOCAL_BENCHMARK_DIR/
fi
