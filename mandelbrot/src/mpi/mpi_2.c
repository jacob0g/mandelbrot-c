#define MAXITER     1000
#ifndef N
#define N	        15000
#endif
#define CHUNK_SIZE  2000 // default

#define BENCHMARK_PATH "benchmarks/task_2"

// MPI Tags
#define TAG_WORK    1
#define TAG_RESULT  2
#define TAG_DONE    3
#define TAG_TIMING  4

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include "timing.h"

// Private Functions
static void master(int chunksize, int n_ranks);
static void worker(int chunksize);

// Timing Globals
static double t_work=0, t_wait=0, t_comm=0;

/* ----------------------------------------------------------------*/

int main(int argc, char* argv[]) {
    MPI_Status  status;
    int         rank, n_ranks;
    int         chunksize = CHUNK_SIZE;
    int         half = (N/2 + 1) * N;


    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);

    // Check master isn't only process
    if (n_ranks < 2)
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

    // Parse arguments
    if (argc == 2)
        chunksize = atoi(argv[1]);

    // Limit chunksize
    if (chunksize > half)
        chunksize = half;

    // Call routines
    if (rank == 0)
        master(chunksize, n_ranks);
    else
        worker(chunksize);

#ifdef BENCHMARK
    DECL_TIMER(t_wait);
    START_TIMER(t_wait);
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(t_wait);

    // Collate times
    if (rank == 0) {
        char filename[256];
        double *times = (double*)malloc(3 * n_ranks * sizeof(double));
        times[0] = t_work;
        times[1] = t_wait;
        times[2] = t_comm;

        for (int r = 1; r < n_ranks; r++) {
            MPI_Recv(times + (3*r), 3, MPI_DOUBLE, r, TAG_TIMING, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        snprintf(
            filename, sizeof(filename), "%s/N_%d-RANK_%d-CHUNKSIZE_%d.dat", 
            BENCHMARK_PATH, N, n_ranks, chunksize
        );

        printf("Writing benchmark times to: %s.\n", filename);
        WRITE_TIMINGS(filename, times, n_ranks);
        free(times);
    }
    else {
        double _times[3] = {t_work, t_wait, t_comm};
        MPI_Send(_times, 3, MPI_DOUBLE, 0, TAG_TIMING, MPI_COMM_WORLD);
    }
#endif

    MPI_Finalize();
}

inline void send_work(int *idx, int rank) {
    MPI_Send(idx, 1, MPI_INT, rank, TAG_WORK, MPI_COMM_WORLD);
}

inline void recv_result(int *result, int *idx, int chunksize, MPI_Status *status) {
    MPI_Recv(idx, 1, MPI_INT, MPI_ANY_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, status);
    MPI_Recv(result + *idx, chunksize, MPI_INT, status->MPI_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

inline void send_done(int rank) {
    MPI_Send(NULL, 0, MPI_BYTE, rank, TAG_DONE, MPI_COMM_WORLD);
}

static void master(int chunksize, int n_ranks) {
    MPI_Status  status;
    int         r, idx, loop;
    int         *n;
    float       *x;
    int         result_idx, n_busy;
    DECL_TIMER(t_work); DECL_TIMER(t_comm);

    int half = (N/2 + 1) * N;
    int padding = (chunksize - half % chunksize) % chunksize;
    float inv_log_maxiter = 1.0f / logf((float)MAXITER);

    n = (int*)malloc((half + padding) * sizeof(int));
    x = (float*)malloc(N*N * sizeof(float));
  
    // Initial work distribution
    START_TIMER(t_comm);
    idx = 0, n_busy = 0;
    for (r = 1; r < n_ranks; r++) {
        send_work(&idx, r);

        n_busy++;
        idx += chunksize;
        if (half <= idx)
            break;
    }
    
    // Signal unassigned workers to complete
    for (r = n_busy + 1; r < n_ranks; r++) {
        send_done(r);
    }

    // Work Scheduling Loop
    while (idx < half || n_busy > 0) {
        recv_result(n, &result_idx, chunksize, &status);

        if (idx < half) {
            // Reply with more work
            send_work(&idx, status.MPI_SOURCE);
            idx += chunksize;
        }
        else {
            send_done(status.MPI_SOURCE);
            n_busy--;
        }
    }
    STOP_TIMER(t_comm);

    START_TIMER(t_work);
    // int row = 0;
    // int col = 0;
    for (idx = 0; idx < half; idx++) {
        float val = logf((float)n[idx]) * inv_log_maxiter;
        x[idx] = val;
        // if (row >= 1 && row < N/2)
            // x[(N - row) * N + col] = val;
        // if (++col == N) { col = 0; row++; }
    }

    // Mirror rows 1..N/2-1 to rows N-1..N/2+1
    for (int j = 1; j < N/2; j++) {
        for (int i = 0; i < N; i++) {
            x[(N - j) * N + i] = x[j * N + i];
        }
    }
    STOP_TIMER(t_work);

#ifdef FILE_IO
    short green, blue;

    printf("Writing mandelbrot.ppm\n");
    FILE *fp = fopen ("mandelbrot.ppm", "w");
    fprintf (fp, "P6\n%4d %4d\n255\n", N, N);
    for (idx = 0; idx < N*N; idx++) { 
        if (x[idx] < 0.5) {
            green = (int)(2 * x[idx] * 255);
            fprintf(fp, "%c%c%c", 255 - green, green, 0);
        } else {
            blue = (int)(2 * x[idx] * 255 - 255);
            fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
        }
    }
    fclose(fp);
#endif

    free(n);
    free(x);
}

static void worker(int chunksize) {
    int         c, i, j, k, n, idx;
    int         loop;
    int         *_n;
    // float       *_zi, *_zj;
    float       zi, zj;
    float       ki, kj;
    MPI_Status  status;
    DECL_TIMER(t_comm); DECL_TIMER(t_work);

    // _zi = (float*)malloc(chunksize * sizeof(float));
    // _zj = (float*)malloc(chunksize * sizeof(float));
    _n  = (int*)malloc(chunksize * sizeof(int));

    while (1) {
        // Receive work header
        START_TIMER(t_comm);
        MPI_Recv(&idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        STOP_TIMER(t_comm);

        // Check if finished
        if (status.MPI_TAG == TAG_DONE) {
            break;
        }

        // Compute work
        START_TIMER(t_work);
        for (loop = 0; loop < chunksize; loop++) {
            c = idx + loop;
            i = c % N;
            j = c / N;

            zi = ki = (4.0f * (i - (float)N/2)) / N;
            zj = kj = (4.0f * (j - (float)N/2)) / N;

            k = 1;
            while (((zi * zi) + (zj * zj) <= 4.0f) && (k++ < MAXITER)) { 
                float new_zi = (zi * zi) - (zj * zj) + ki;
                zj = 2.0f * zi * zj + kj;
                zi = new_zi;
            }
          
            _n[loop] = k;
        }
        STOP_TIMER(t_work);

        // Reply with result
        START_TIMER(t_comm);
        MPI_Send(&idx, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        MPI_Send(_n, chunksize, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        STOP_TIMER(t_comm);
    }

    // free(_zi);
    // free(_zj);
    free(_n);
}

