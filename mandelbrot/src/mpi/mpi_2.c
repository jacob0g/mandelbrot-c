#define MAXITER     1000
#define N	        2000
#define CHUNK_SIZE  100000 // default

// MPI Tags
#define TAG_WORK    1
#define TAG_RESULT  2
#define TAG_DONE    3

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Private Functions
static void master(int chunksize, int n_ranks);
static void worker(int chunksize);

/* ----------------------------------------------------------------*/

int main(int argc, char* argv[]) {
    MPI_Status  status;
    int         rank, n_ranks;
    int         chunksize = CHUNK_SIZE;

    // MPI Initialisation
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);

    // Parse arguments
    if (argc == 2)
        chunksize = atoi(argv[1]);
    
    // Call routines
    if (rank == 0)
        master(chunksize, n_ranks);
    else
        worker(chunksize);
    
    MPI_Finalize();
}

inline void send_work(float *zi, float *zj, int *idx, int chunksize, int rank) {
    MPI_Send(idx, 1, MPI_INT, rank, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(zi, chunksize, MPI_FLOAT, rank, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(zj, chunksize, MPI_FLOAT, rank, TAG_WORK, MPI_COMM_WORLD);
}

inline void send_done(int rank) {
    MPI_Send(NULL, 0, MPI_BYTE, rank, TAG_DONE, MPI_COMM_WORLD);
}

static void master(int chunksize, int n_ranks) {
    MPI_Status  status;
    int         i, j, k, r, idx;
    float       *x, *zi, *zj;
    int         result_idx, n_busy;
    int         half = (N/2 + 1) * N;
    int         padding = half % chunksize;

    // Check master isn't only process
    if (n_ranks < 2)
        return;

    zi = (float*)malloc((half + padding) * sizeof(float));
    zj = (float*)malloc((half + padding) * sizeof(float));
    x  = (float*)malloc(N*N * sizeof(float));
  
    // Initialize z
    for (idx = 0; idx < half; idx++) {
        i = idx % N;
        j = idx / N;

        zi[idx] = (4.0 * (i - (float)N/2)) / N;
        zj[idx] = (4.0 * (j - (float)N/2)) / N;
    }

    double t0 = MPI_Wtime();

    // Initial work distribution
    idx = 0, n_busy = 0;
    for (r = 1; r < n_ranks; r++) {
        send_work(&zi[idx], &zj[idx], &idx, chunksize, r);

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
        // Receive a result
        MPI_Recv(&result_idx, 1, MPI_INT, MPI_ANY_SOURCE,
                TAG_RESULT, MPI_COMM_WORLD, &status);
        MPI_Recv(&x[result_idx], chunksize, MPI_FLOAT, status.MPI_SOURCE,
                TAG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (idx < half) {
            // Reply with more work
            send_work(&zi[idx], &zj[idx], &idx, chunksize, status.MPI_SOURCE);
            idx += chunksize;
        }
        else {
            send_done(status.MPI_SOURCE);
            n_busy--;
        }
    }

    // Mirror rows 1..N/2-1 to rows N-1..N/2+1
    for (j = 1; j < N/2; j++) {
        for (i = 0; i < N; i++) {
            x[(N - j) * N + i] = x[j * N + i];
        }
    }

#ifdef FILE_IO
    int green, blue, loop;

    printf("Writing mandelbrot.ppm\n");
    FILE *fp = fopen ("mandelbrot.ppm", "w");
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

    free(zi);
    free(zj);
    free(x);
}

static void worker(int chunksize) {
    int         idx, k, n;
    float       *_zi, *_zj, *_x;
    float       ki, kj;
    MPI_Status  status;

    _zi = (float*)malloc(chunksize * sizeof(float));
    _zj = (float*)malloc(chunksize * sizeof(float));
    _x  = (float*)malloc(chunksize * sizeof(float));

    while (1) {
        // Receive work header
        MPI_Recv(&idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == TAG_DONE)
            break;

        // Receive rest of work payload
        MPI_Recv(_zi, chunksize, MPI_FLOAT, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(_zj, chunksize, MPI_FLOAT, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Compute work
        for (n = 0; n < chunksize; n++) {
            ki = _zi[n];
            kj = _zj[n];

            k = 1;
            while (((_zi[n] * _zi[n]) + (_zj[n] * _zj[n]) <= 4) && (k++ < MAXITER)) { 
                float new_zi = (_zi[n] * _zi[n]) - (_zj[n] * _zj[n]) + ki;
                _zj[n] = 2 * _zi[n] * _zj[n] + kj;
                _zi[n] = new_zi;
            }
          
            _x[n] = log((float)k) / log((float)MAXITER);
        }

        // Reply with result
        MPI_Send(&idx, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        MPI_Send(_x, chunksize, MPI_FLOAT, 0, TAG_RESULT, MPI_COMM_WORLD);
    }

    free(_zi);
    free(_zj);
    free(_x);
}

