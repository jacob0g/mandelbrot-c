#define MAXITER 1000
#define N	    2000

#define BENCHMARK_PATH "benchmarks/task_3"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>
#include <math.h>
#include "timing.h"


/* ----------------------------------------------------------------*/

int main(int argc, char* argv[]) {
    MPI_Status status;
    int rank, n_ranks;

    int	   i, j, loop;
    short  green, blue;
    float  *x, *zi, *zj;
    float  *_zi, *_zj;
    uint16_t k;
    uint16_t *n, *_n;
    size_t c;
    float  ki, kj;
    int    half = (N/2 + 1) * N;
    double t_comm=0, t_wait=0, t_work=0;
    DECL_TIMER(t_comm); DECL_TIMER(t_wait); DECL_TIMER(t_work);
    FILE   *fp;

    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);

    int chunksize = half / n_ranks;

    // Root initialises z
    if (rank == 0) {
        zi = (float*)malloc(half * sizeof(float));
        zj = (float*)malloc(half * sizeof(float));
        x = (float*)malloc(N*N * sizeof(float));
        n = (uint16_t*)malloc(half * sizeof(uint16_t));
      
        c = 0;
        for (int idx = 0; idx < chunksize; idx++) {
            for (int block_idx = 0; block_idx < n_ranks; block_idx++) {
                loop = (block_idx * chunksize) + idx;
                i = loop % N;
                j = loop / N;

                zi[c] = (4.0 * (i - (float)N/2)) / N;
                zj[c++] = (4.0 * (j - (float)N/2)) / N;
            }
        }
    }

    // Local z points
    _zi = (float*)malloc(chunksize * sizeof(float));
    _zj = (float*)malloc(chunksize * sizeof(float));
    _n = (uint16_t*)malloc(chunksize * sizeof(uint16_t));

    START_TIMER(t_wait);
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(t_wait);

    // Scatter points to processes
    START_TIMER(t_comm);
    MPI_Scatter(zi, chunksize, MPI_FLOAT, _zi, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD); 
    MPI_Scatter(zj, chunksize, MPI_FLOAT, _zj, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD); 
    STOP_TIMER(t_comm);

    START_TIMER(t_work);
    for (loop = 0; loop < chunksize; loop++) {
        ki = _zi[loop];
        kj = _zj[loop];

        k = 1;
        while (((_zi[loop] * _zi[loop]) + (_zj[loop] * _zj[loop]) <= 4) && (k++ < MAXITER)) { 
            float new_zi = (_zi[loop] * _zi[loop]) - (_zj[loop] * _zj[loop]) + ki;
            _zj[loop] = 2 * _zi[loop] * _zj[loop] + kj;
            _zi[loop] = new_zi;
        }
      
        _n[loop] = k;
        // _n[loop] = log((float)k) / log((float)MAXITER);
    }
    STOP_TIMER(t_work);

    // Wait for processes
    START_TIMER(t_wait);
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(t_wait);

    // Compile number of iterations
    START_TIMER(t_comm);
    MPI_Gather(_n, chunksize, MPI_UINT16_T, n, chunksize, MPI_UINT16_T, 0, MPI_COMM_WORLD);
    STOP_TIMER(t_comm);

    if (rank == 0) {
        // Re-order cyclic partitioned results to original order
        c = 0;
        for (int idx = 0; idx < chunksize; idx++) {
            for (int block_idx = 0; block_idx < n_ranks; block_idx++) {
                loop = (block_idx * chunksize) + idx;
                x[loop] = log((float)n[c++]) / log((float)MAXITER);
            }
        }
        // for (loop = 0; loop < half; loop++) {
        //     x[loop] = log((float)n[loop]) / log((float)MAXITER);
        // }

        // Mirror rows 1..N/2-1 to rows N-1..N/2+1
        for (j = 1; j < N/2; j++) {
            for (i = 0; i < N; i++) {
                x[(N - j) * N + i] = x[j * N + i];
            }
        }

/* ----------------------------------------------------------------*/
  
#ifdef FILE_IO
        printf("Writing mandelbrot.ppm\n");
        fp = fopen ("mandelbrot.ppm", "w");
        fprintf (fp, "P6\n%4d %4d\n255\n", N, N);
        
        for (loop = 0; loop < N*N; loop++) { 
            if (x[loop] < 0.5) {
                green = (int)(2 * x[loop] * 255);
                fprintf(fp, "%c%c%c", 255 - green, green, 0);
            } else {
                blue = (int)(2 * x[loop] * 255 - 255);
                fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
            }
        }
        
        fclose(fp);
#endif

/* ----------------------------------------------------------------*/

        free(zi);
        free(zj);
        free(n);
        free(x);
    }

    free(_zi);
    free(_zj);
    free(_n);

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

