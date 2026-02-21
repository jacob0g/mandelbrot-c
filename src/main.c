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
    FILE   *fp;

    float complex   z, kappa;

    x = (float*)malloc(N * N * sizeof(float));
  
    for (loop = 0; loop < N * N; loop++) {
	    i = loop % N;
	    j = loop / N;

	    z = kappa = (4.0 * (i - N / 2)) / N + (4.0 * (j - N / 2)) / N * I;
	
	    k = 1;
	    while ((cabs(z) <= 2) && (k++ < MAXITER)) 
	        z = z * z + kappa;
	  
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

