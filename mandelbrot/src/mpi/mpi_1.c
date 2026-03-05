#define MAXITER 1000
#define N	    2000

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

/* ----------------------------------------------------------------*/

int main(int argc, char* argv[]) {
    MPI_Status status;
    int rank, ncores;

    int	   i, j, k, loop;
    short  green, blue;
    float  *x, *zi, *zj;
    float  ki, kj;
    int    half = (N/2 + 1) * N;
    float  *_x, *_zi, *_zj;
    double t, t_comm=0, t_wait=0, t_work=0;
    FILE   *fp;

    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ncores);

    int chunksize = half / ncores;

    // Root initialises z
    if (rank == 0) {
        zi = (float*)malloc(half * sizeof(float));
        zj = (float*)malloc(half * sizeof(float));
        x = (float*)malloc(N*N * sizeof(float));
      
        for (loop = 0; loop < half; loop++) {
            i = loop % N;
            j = loop / N;

            zi[loop] = (4.0 * (i - (float)N/2)) / N;
            zj[loop] = (4.0 * (j - (float)N/2)) / N;
        }
    }

    // Local z points
    _zi = (float*)malloc(chunksize * sizeof(float));
    _zj = (float*)malloc(chunksize * sizeof(float));
    _x = (float*)malloc(chunksize * sizeof(float));

    MPI_Barrier(MPI_COMM_WORLD);

    // Scatter points to processes
    t = MPI_Wtime();
    MPI_Scatter(zi, chunksize, MPI_FLOAT, _zi, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD); 
    MPI_Scatter(zj, chunksize, MPI_FLOAT, _zj, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD); 
    t_comm += MPI_Wtime() - t;

    t = MPI_Wtime();
    for (loop = 0; loop < chunksize; loop++) {
        ki = _zi[loop];
        kj = _zj[loop];

        k = 1;
        while (((_zi[loop] * _zi[loop]) + (_zj[loop] * _zj[loop]) <= 4) && (k++ < MAXITER)) { 
            float new_zi = (_zi[loop] * _zi[loop]) - (_zj[loop] * _zj[loop]) + ki;
	        _zj[loop] = 2 * _zi[loop] * _zj[loop] + kj;
	        _zi[loop] = new_zi;
        }
	  
	    _x[loop] = log((float)k) / log((float)MAXITER);
    }
    t_work += MPI_Wtime() - t;

    // Wait for processes
    t = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    t_wait += MPI_Wtime() - t;

    // Compile results
    t = MPI_Wtime();
    MPI_Gather(_x, chunksize, MPI_FLOAT, x, chunksize, MPI_FLOAT, 0, MPI_COMM_WORLD);
    t_comm += MPI_Wtime() - t;

    if (rank == 0) {
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
        free(x);
    }

    free(_zi);
    free(_zj);
    free(_x);

#ifdef BENCHMARK
    // Collate Benchmark Times
    double _times[3] = {t_work, t_wait, t_comm};
    double *times;

    if (rank == 0)
        times = (double*)malloc(ncores * 3 * sizeof(double));

    MPI_Gather(_times, 3, MPI_DOUBLE, times, 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Writing benchmark times.\n");
        fp = fopen("benchmarks/task_1/times.dat", "w");
        fprintf(fp, "# rank\tt_work\t\tt_wait\t\tt_comm\n");

        for (int i = 0; i < ncores*3; i += 3) {
            fprintf(fp, "  %d \t%lf\t%lf\t%lf\n", i/3, times[i], times[i+1], times[i+2]);
        }

        fclose(fp);
    }
#endif

    MPI_Finalize();
}

