#define MAXITER 1000
#define N	    4000

#ifdef FILE_IO
#include <stdio.h>
#endif

#include <stdlib.h>
#include <complex.h>
#include <math.h>

/* ----------------------------------------------------------------*/

int main() {
    int	   i, j, k, loop;
    float  *x;

    float complex   z, kappa;

    x = (float*)malloc(N * N * sizeof(float));
  
    for (loop = 0; loop < N * N; loop++) {
	    i = loop % N;
	    j = loop / N;

	    z = kappa = (4.0f * (i - N/2)) / N + (4.0f * (j - N/2))/N * I;
    
	    k = 1;
        while ((cabsf(z) <= 2.0f) && (k++ < MAXITER)) 
	        z = z * z + kappa;

        x[loop] = logf((float)k) / logf((float)MAXITER);
    }

/* ----------------------------------------------------------------*/
  
#ifdef FILE_IO
    short  green, blue;
    printf("Writing mandelbrot.ppm\n");
    FILE *fp = fopen ("mandelbrot.ppm", "w");
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
#endif

/* ----------------------------------------------------------------*/

    free(x);
}

