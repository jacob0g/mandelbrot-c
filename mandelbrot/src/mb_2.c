#define MAXITER 1000
#define N	2000

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ----------------------------------------------------------------*/

int main() {
    int	   i, j, k, loop;
    short  green, blue;
    float  *x;
    float *zi, *zj;
    FILE   *fp;
    float ki, kj, z_squared;


    zi = (float*)malloc(N*N * sizeof(float));
    zj = (float*)malloc(N*N * sizeof(float));

    x = (float*)malloc(N*N * sizeof(float));
  
    // Initialise z
    for (loop = 0; loop < N*N; loop++) {
        // Z_i
        i = loop % N;
        zi[loop] = (4.0 * (i - (float)N/2)) / N;

        // Z_j
        j = loop / N;
        zj[loop] = (4.0 * (j - (float)N/2)) / N;
    }

    for (loop = 0; loop < N * N; loop++) {
        ki = zi[loop];
        kj = zj[loop];
	
	    k = 1;
	    while ((zi[loop] + zj[loop] <= 4) && (k++ < MAXITER)) { 
	        zi[loop] = (zi[loop] * zi[loop]) - (zj[loop] * zj[loop]) + ki;
	        zj[loop] = 2 * zi[loop] * zj[loop] + kj;
        }
	  
	    x[loop] = log((float)k) / log((float)MAXITER);
    }

/* ----------------------------------------------------------------*/
  
    printf("Writing mandelbrot.ppm\n");
    fp = fopen ("mandelbrot.ppm", "w");
    fprintf (fp, "P6\n%4d %4d\n255\n", N, N);
    
    for (loop=0; loop<N*N; loop++) 
	    if (x[loop]<0.5) {
	        green= (int) (2*x[loop]*255);
                fprintf (fp, "%c%c%c", 255-green,green,0);
	    } else {
	        blue= (int) (2*x[loop]*255-255);
                fprintf (fp, "%c%c%c", 0,255-blue,blue);
	    }
    
    fclose(fp);

/* ----------------------------------------------------------------*/

    free(x);
}

