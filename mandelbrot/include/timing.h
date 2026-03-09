#ifndef TIMING_H
#define TIMING_H

#include <stdio.h>
#include <mpi.h>

// Declare timer variables once per scope before use.
#define DECL_TIMER(acc)  double _timer_##acc

// Accumulates elapsed wall-clock time between START_TIMER / STOP_TIMER into `acc`.
#define START_TIMER(acc) _timer_##acc = MPI_Wtime()
#define STOP_TIMER(acc)  (acc) += MPI_Wtime() - _timer_##acc

// Writes per-rank timing data (t_work, t_wait, t_comm) to a file.
#define WRITE_TIMINGS(filepath, times, n_ranks) do { \
    FILE *_timing_fp = fopen((filepath), "w"); \
    if (_timing_fp) { \
        fprintf(_timing_fp, "# rank\tt_work\t\tt_wait\t\tt_comm\n"); \
        for (int _i = 0; _i < (n_ranks) * 3; _i += 3) { \
            fprintf(_timing_fp, "  %d \t%lf\t%lf\t%lf\n", \
                    _i / 3, (times)[_i], (times)[_i+1], (times)[_i+2]); \
        } \
        fclose(_timing_fp); \
    } else { \
        fprintf(stderr, "Error: could not open '%s' for writing.\n", \
                (filepath)); \
    } \
} while (0)

#endif
