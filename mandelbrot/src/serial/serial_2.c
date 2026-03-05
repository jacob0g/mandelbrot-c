#define MAXITER 1000
#define N	    2000

#ifdef FILE_IO
#include <stdio.h>
#endif

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
    for (j = N/2 - 1; j >= 0; j--) {
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

    free(zi);
    free(zj);
    free(x);
}

