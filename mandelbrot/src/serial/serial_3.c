#define MAXITER 1000
#define N	    2000

#ifdef FILE_IO
#include <stdio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ----------------------------------------------------------------*/

int main() {
    int	   i, j, k, loop;
    short  green, blue;
    float  *x, *zi, *zj;
    float  ki, kj;

#ifdef FILE_IO
    FILE   *fp;
#endif

    zi = (float*)malloc(N*N/2 * sizeof(float));
    zj = (float*)malloc(N*N/2 * sizeof(float));
    x = (float*)malloc(N*N/2 * sizeof(float));
 
    float *_zi, *_zj, *_x;
    _zi = (float*)malloc(N*N * sizeof(float));
    _zj = (float*)malloc(N*N * sizeof(float));
    _x = (float*)malloc(N*N * sizeof(float));
  
    // Initialise z
    for (loop = 0; loop < N*N/2; loop++) {
        i = loop % N;
        j = loop / N;

        zi[loop] = (4.0 * (i - (float)N/2)) / N;
        zj[loop] = (4.0 * (j - (float)N/2)) / N;
    }

    for (loop = 0; loop < N*N/2; loop++) {
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

    for (loop = 0; loop < N*N; loop++) {
        i = loop % N;
        j = loop / N;

        _zi[loop] = (4.0 * (i - (float)N/2)) / N;
        _zj[loop] = (4.0 * (j - (float)N/2)) / N;
    }

    for (loop = 0; loop < N*N; loop++) {
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

    int icount, jcount = 0;
    int _loop = 0;
    for (loop = 0; loop < N*N/2; loop++) { 
        if (x[loop] != _x[_loop]) {
            printf("x[%d] = %f; _x[%d] = %f\n", loop, x[loop], _loop, _x[_loop]);
            printf("zi = %f, zj = %f, _zi = %f, _zj = %f\n", zi[loop], zj[loop], _zi[_loop], _zj[_loop]);
        }

        _loop++;
    }
    for (j = N/2 - 1; j >= 0; j--) {
        for (i = 0; i < N; i++) {
            loop = j * N + i;
            if (x[loop] != _x[_loop]) {
                printf("x[%d] = %f; _x[%d] = %f\n", loop, x[loop], _loop, _x[_loop]);
                printf("zi = %f, zj = %f, _zi = %f, _zj = %f\n", zi[loop], zj[loop], _zi[_loop], _zj[_loop]);
            }

            if (zi[loop] != _zi[_loop])
                icount++;

            if (zj[loop] != _zj[_loop])
                jcount++;

            _loop++;
        }
    }
    printf("i count: %d\n", icount);
    printf("j count: %d\n", jcount);
     
/* ----------------------------------------------------------------*/
  
#ifdef FILE_IO
    printf("Writing mandelbrot.ppm\n");
    fp = fopen ("mandelbrot.ppm", "w");
    fprintf (fp, "P6\n%4d %4d\n255\n", N, N);
    
    for (loop = 0; loop < N*N/2; loop++) { 
	    if (x[loop] < 0.5) {
	        green = (int)(2 * x[loop] * 255);
            fprintf(fp, "%c%c%c", 255 - green, green, 0);
	    } else {
	        blue = (int)(2 * x[loop] * 255 - 255);
            fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
	    }
    }
    for (int j = 0; j < N/2; j++) {
        int j_inv = (N/2 - 1) - j;
        for (int i = 0; i < N; i++) {
            loop = (j_inv * N) + i;
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

    free(zi);
    free(zj);
    free(x);
}

