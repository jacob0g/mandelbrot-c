#define MAXITER 1000
#define N	    8000

#ifdef FILE_IO
#include <stdio.h>
#endif

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

#ifdef FILE_IO
    FILE   *fp;
#endif

    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ncores);

    zi = (float*)malloc(half * sizeof(float));
    zj = (float*)malloc(half * sizeof(float));
    x = (float*)malloc(N*N * sizeof(float));
  
    // Initialise z
    for (loop = 0; loop < half; loop++) {
        i = loop % N;
        j = loop / N;

        zi[loop] = (4.0 * (i - (float)N/2)) / N;
        zj[loop] = (4.0 * (j - (float)N/2)) / N;
    }

    for (loop = 0; loop < half; loop++) {
        ki = zi[loop];
        kj = zj[loop];

        k = 1;
        while (((zi[loop] * zi[loop]) + (zj[loop] * zj[loop]) <= 4) && (k++ < MAXITER)) { 
            float new_zi = (zi[loop] * zi[loop]) - (zj[loop] * zj[loop]) + ki;
	        zj[loop] = 2 * zi[loop] * zj[loop] + kj;
	        zi[loop] = new_zi;
        }
	  
	    x[loop] = log((float)k) / log((float)MAXITER);
    }

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

    MPI_Finalize();

    free(zi);
    free(zj);
    free(x);
}

