#define MAXITER     1000
#define N	        2000
#define CHUNK_SIZE  100000 // default

// MPI Tags
#define TAG_WORK    1
#define TAG_RESULT  2
#define TAG_DONE    3
#define TAG_TIMING  4

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Timing Macro
#define TIMETHIS(acc, ...) do { \
    double _t0 = MPI_Wtime(); \
    __VA_ARGS__ \
    (acc) += MPI_Wtime() - _t0; \
} while (0)

// Private Functions
static void master(int chunksize, int n_ranks);
static void worker(int chunksize);

// Timing Globals
static double t0, t_work=0, t_wait=0, t_comm=0;

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
    
#ifdef BENCHMARK
    t0 = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    t_wait += MPI_Wtime() - t0;

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

        // Write times
        printf("Writing benchmark times.\n");
        snprintf(filename, sizeof(filename), "benchmarks/task_2/chunksize-%d.dat", chunksize);
        FILE *fp = fopen(filename, "w");

        fprintf(fp, "# rank\tt_work\t\tt_wait\t\tt_comm\n");
        for (int i = 0; i < n_ranks*3; i += 3) {
            fprintf(fp, "  %d \t%lf\t%lf\t%lf\n", i/3, times[i], times[i+1], times[i+2]);
        }

        fclose(fp);
        free(times);
    }
    else {
        double _times[3] = {t_work, t_wait, t_comm};
        MPI_Send(_times, 3, MPI_DOUBLE, 0, TAG_TIMING, MPI_COMM_WORLD);
    }
#endif

    MPI_Finalize();
}

inline void send_work(float *zi, float *zj, int *idx, int chunksize, int rank) {
    MPI_Send(idx, 1, MPI_INT, rank, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(zi, chunksize, MPI_FLOAT, rank, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(zj, chunksize, MPI_FLOAT, rank, TAG_WORK, MPI_COMM_WORLD);
}

inline void recv_result(float *result, int *idx, int chunksize, MPI_Status *status) {
    MPI_Recv(idx, 1, MPI_INT, MPI_ANY_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, status);
    MPI_Recv(result + *idx, chunksize, MPI_FLOAT, status->MPI_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
    x  = (float*)malloc(N * N * sizeof(float));
  
    // Initialize z
TIMETHIS(t_work,
    for (idx = 0; idx < half; idx++) {
        i = idx % N;
        j = idx / N;

        zi[idx] = (4.0 * (i - (float)N/2)) / N;
        zj[idx] = (4.0 * (j - (float)N/2)) / N;
    }
);

    t0 = MPI_Wtime();
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
        recv_result(x, &result_idx, chunksize, &status);

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
    t_comm += MPI_Wtime() - t0;

    t0 = MPI_Wtime();
    // Mirror rows 1..N/2-1 to rows N-1..N/2+1
    for (j = 1; j < N/2; j++) {
        for (i = 0; i < N; i++) {
            x[(N - j) * N + i] = x[j * N + i];
        }
    }
    t_work += MPI_Wtime() - t0;

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
        t0 = MPI_Wtime();
        // Receive work header
        MPI_Recv(&idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == TAG_DONE) {
            t_comm += MPI_Wtime() - t0;
            break;
        }

        // Receive rest of work payload
        MPI_Recv(_zi, chunksize, MPI_FLOAT, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(_zj, chunksize, MPI_FLOAT, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        t_comm += MPI_Wtime() - t0;

        t0 = MPI_Wtime();
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
        t_work += MPI_Wtime() - t0;

        t0 = MPI_Wtime();
        // Reply with result
        MPI_Send(&idx, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        MPI_Send(_x, chunksize, MPI_FLOAT, 0, TAG_RESULT, MPI_COMM_WORLD);
        t_work += MPI_Wtime() - t0;
    }

    free(_zi);
    free(_zj);
    free(_x);
}

