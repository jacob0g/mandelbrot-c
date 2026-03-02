#define MAXITER 1000
#define N	2000

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

/* ----------------------------------------------------------------*/

int main() {
    int	   i, j, k, loop;
    short  green, blue;
    float  *x;
    float complex *z;
    FILE   *fp;

    float complex   kappa;

    z = (float complex*)malloc(N * N * sizeof(float complex*));
    x = (float*)malloc(N * N * sizeof(float));
  
    // Initialise z
    for (loop = 0; loop < N*N; loop++) {
        i = loop % N;
        j = loop / N;

        z[loop] = (4.0 * (i - (float)N/2))/N + (4.0 * (j - (float)N/2))/N * I;
    }

    for (loop = 0; loop < N * N; loop++) {
        kappa = z[loop];
	
	    k = 1;
	    while ((cabs(z[loop]) <= 2) && (k++ < MAXITER)) 
	        z[loop] = z[loop] * z[loop] + kappa;
	  
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

