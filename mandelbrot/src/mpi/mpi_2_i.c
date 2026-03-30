#define MAXITER     1000
#ifndef N
#define N	        15000
#endif
#define CHUNK_SIZE  2000 // default

#define BENCHMARK_PATH "benchmarks/task_2_i"

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

inline void send_work(int *idx, int rank, MPI_Request *req) {
    MPI_Isend(idx, 1, MPI_INT, rank, TAG_WORK, MPI_COMM_WORLD, req);
}

inline void recv_result(int *result, int *idx, int chunksize, MPI_Status *status, MPI_Request *req) {
    MPI_Recv(idx, 1, MPI_INT, MPI_ANY_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, status);
    MPI_Irecv(result + *idx, chunksize, MPI_INT, status->MPI_SOURCE,
            TAG_RESULT, MPI_COMM_WORLD, req);
}

inline void send_done(int rank, MPI_Request *req) {
    MPI_Isend(NULL, 0, MPI_BYTE, rank, TAG_DONE, MPI_COMM_WORLD, req);
}

static void master(int chunksize, int n_ranks) {
    MPI_Status  status;
    MPI_Request req;
    MPI_Request *requests;
    int         r, idx, loop;
    int         *n;
    float       *x;
    int         result_idx, n_busy, src_idx;
    DECL_TIMER(t_work); DECL_TIMER(t_comm); DECL_TIMER(t_wait);

    int n_slaves = n_ranks - 1;
    int half = (N/2 + 1) * N;
    int padding = (chunksize - half % chunksize) % chunksize;

    n = (int*)malloc((half + padding) * sizeof(int));
    x = (float*)malloc(N*N * sizeof(float));
  
    requests = (MPI_Request*)malloc(n_slaves * sizeof(MPI_Request));

    // Initial work distribution
    START_TIMER(t_comm);
    idx = 0, n_busy = 0;
    for (r = 1; r < n_ranks; r++) {
        send_work(&idx, r, &requests[r - 1]);

        n_busy++;
        idx += chunksize;
        if (half <= idx)
            break;
    }
    
    // Signal unassigned workers to complete
    for (r = n_busy + 1; r < n_ranks; r++) {
        send_done(r, &requests[r - 1]);
    }
    STOP_TIMER(t_comm);

    START_TIMER(t_wait);
    MPI_Waitall(n_slaves, requests, MPI_STATUSES_IGNORE);
    STOP_TIMER(t_wait);

    // Work Scheduling Loop
    while (idx < half || n_busy > 0) {
        START_TIMER(t_comm);
        MPI_Recv(&result_idx, 1, MPI_INT, MPI_ANY_SOURCE,
                TAG_RESULT, MPI_COMM_WORLD, &status);
        src_idx = status.MPI_SOURCE;
        MPI_Irecv(&n[result_idx], chunksize, MPI_INT, src_idx,
                TAG_RESULT, MPI_COMM_WORLD, &req);

        // Is there more work left?
        if (idx < half) {
            // Reply with more work
            send_work(&idx, src_idx, &requests[src_idx - 1]);
            idx += chunksize;
        }
        else {
            send_done(src_idx, &requests[src_idx - 1]);
            n_busy--;
        }
        STOP_TIMER(t_comm);

        // Wait for slave to receive reply
        MPI_Request reqs[2] = {req, requests[src_idx - 1]};
        START_TIMER(t_wait);
        MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
        STOP_TIMER(t_wait);
    }

    START_TIMER(t_work);
    for (idx = 0; idx < half; idx++) {
        x[idx] = log((float)n[idx]) / log((float)MAXITER);
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

    free(requests);
    free(n);
    free(x);
}

static void worker(int chunksize) {
    int         c, i, j, k, n, idx, next_idx;
    int         loop;
    int         *_n;
    float       *_zi, *_zj;
    float       ki, kj;
    MPI_Status  _statuses[3];
    MPI_Request _req1, _req2, _req_next;
    DECL_TIMER(t_work); DECL_TIMER(t_comm); DECL_TIMER(t_wait);


    _zi = (float*)malloc(chunksize * sizeof(float));
    _zj = (float*)malloc(chunksize * sizeof(float));
    _n  = (int*)malloc(chunksize * sizeof(int));

    // Receive initial work header
    START_TIMER(t_comm);
    MPI_Recv(&idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &_statuses[0]);
    STOP_TIMER(t_comm);

    if (_statuses[0].MPI_TAG == TAG_DONE) {
        goto end;
    }

    while (1) {
        // Start receiving next index
        START_TIMER(t_comm);
        MPI_Irecv(&next_idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &_req_next);
        STOP_TIMER(t_comm);

        // Compute work
        START_TIMER(t_work);
        for (loop = 0; loop < chunksize; loop++) {
            c = idx + loop;
            i = c % N;
            j = c / N;

            _zi[loop] = ki = (4.0 * (i - (float)N/2)) / N;
            _zj[loop] = kj = (4.0 * (j - (float)N/2)) / N;

            k = 1;
            while (((_zi[loop] * _zi[loop]) + (_zj[loop] * _zj[loop]) <= 4) && (k++ < MAXITER)) { 
                float new_zi = (_zi[loop] * _zi[loop]) - (_zj[loop] * _zj[loop]) + ki;
                _zj[loop] = 2 * _zi[loop] * _zj[loop] + kj;
                _zi[loop] = new_zi;
            }
          
            _n[loop] = k;
        }
        STOP_TIMER(t_work);

        // Reply with result
        START_TIMER(t_comm);
        MPI_Isend(&idx, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD, &_req1);
        MPI_Isend(_n, chunksize, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD, &_req2);
        STOP_TIMER(t_comm);

        MPI_Request _reqs[3] = {_req_next, _req1, _req2};
        START_TIMER(t_wait);
        MPI_Waitall(3, _reqs, _statuses); 
        STOP_TIMER(t_wait);

        // Check if finished
        if (_statuses[0].MPI_TAG == TAG_DONE) {
            break;
        }
        idx = next_idx;
    }

end:
    free(_zi);
    free(_zj);
    free(_n);
}

