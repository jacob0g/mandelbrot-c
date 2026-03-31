#define MAXITER     1000
#ifndef N
#define N           15000
#endif

#define BENCHMARK_PATH "benchmarks/task_1"

#if defined(FILE_IO) || defined(BENCHMARK)
#include <stdio.h>
#endif
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include "timing.h"


/* ----------------------------------------------------------------*/

int main(int argc, char* argv[]) {
    MPI_Status status;
    int rank, n_ranks;

    int     i, j, k, loop;
    int     *n, *_n;
    float   *x, *_x;
    float   ki, kj, zi, zj, new_zi;
    double  t_work, t_comm, t_wait;
    DECL_TIMER(t_work); DECL_TIMER(t_comm); DECL_TIMER(t_wait);

    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);

    int half = (N/2 + 1) * N;
    int chunksize = half / n_ranks;
    const float inv_log_maxiter = 1.0f / logf((float)MAXITER);

    // Root initialises point indexes
    if (rank == 0) {
        x = (float*)malloc(half * sizeof(float));
        n = (int*)malloc(half * sizeof(int));
      
        for (loop = 0; loop < half; loop++) {
            n[loop] = loop;
        }
    }

    // Allocate local buffers
    _x = (float*)malloc(chunksize * sizeof(float));
    _n = (int*)malloc(chunksize * sizeof(int));

    START_TIMER(t_wait);
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(t_wait);

    // Scatter points to processes
    START_TIMER(t_comm);
    MPI_Scatter(n, chunksize, MPI_INT, _n, chunksize, MPI_INT, 0, MPI_COMM_WORLD); 
    STOP_TIMER(t_comm);

    START_TIMER(t_work);
    for (loop = 0; loop < chunksize; loop++) {
        i = _n[loop] % N;
        j = _n[loop] / N;

        ki = zi = (4.0f * (i - (float)N/2)) / N;
        kj = zj = (4.0f * (j - (float)N/2)) / N;

        k = 1;
        while (((zi * zi) + (zj * zj) <= 4.0f) && (k++ < MAXITER)) { 
            new_zi = (zi * zi) - (zj * zj) + ki;
            zj = 2.0f * zi * zj + kj;
            zi = new_zi;
        }
      
        _x[loop] = logf((float)k) * inv_log_maxiter;
    }
    STOP_TIMER(t_work);

    // Wait for processes
    START_TIMER(t_wait);
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(t_wait);

    // Compile number of iterations
    START_TIMER(t_comm);
    MPI_Gather(_x, chunksize, MPI_FLOAT, x, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD);
    STOP_TIMER(t_comm);

    if (rank == 0) {

/* ----------------------------------------------------------------*/
  
#ifdef FILE_IO
        short green, blue;

        printf("Writing mandelbrot.ppm\n");
        FILE *fp = fopen ("mandelbrot.ppm", "w");
        fprintf (fp, "P6\n%4d %4d\n255\n", N, N);
        
        // Write top half
        for (loop = 0; loop < half; loop++) { 
            if (x[loop] < 0.5) {
                green = (int)(2 * x[loop] * 255);
                fprintf(fp, "%c%c%c", 255 - green, green, 0);
            } else {
                blue = (int)(2 * x[loop] * 255 - 255);
                fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
            }
        }
        // Write bottom half (top half mirrored)
        for (j = N/2 - 1; j >= 1; j--) {
            for (i = 0; i < N; i++) {
                loop = j * N + i;
                if (x[loop] < 0.5) {
                    green = (int)(2 * x[loop] * 255);
                    fprintf(fp, "%c%c%c", 255 - green, green, 0);
                } else {
                    blue = (int)(2 * x[loop] * 255 - 255);
                    fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
                }
            }
        }       
        fclose(fp);
#endif

/* ----------------------------------------------------------------*/

        free(n);
        free(x);
    }

    free(_n);
    free(_x);

#ifdef BENCHMARK
    // Collate Benchmark Times
    double _times[3] = {t_work, t_wait, t_comm};
    double *times;

    if (rank == 0)
        times = (double*)malloc(n_ranks * 3 * sizeof(double));

    MPI_Gather(_times, 3, MPI_DOUBLE, times, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        char filename[256];
        snprintf(
            filename, sizeof(filename), "%s/N_%d-RANK_%d.dat", 
            BENCHMARK_PATH, N, n_ranks
        );

        printf("Writing benchmark times to: %s.\n", filename);
        WRITE_TIMINGS(filename, times, n_ranks);
        free(times);
    }
#endif

    MPI_Finalize();
}

