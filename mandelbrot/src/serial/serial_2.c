#define MAXITER 1000
#define N	    4000

#ifdef FILE_IO
#include <stdio.h>
#endif

#include <stdlib.h>
#include <math.h>

/* ----------------------------------------------------------------*/

int main() {
    int	   i, j, k, loop;
    float  *x;
    float  ki, kj, zi, zj, new_zi;
    int    half = (N/2 + 1) * N;

    x = (float*)malloc(half * sizeof(float));
  
    // Iterate points
    for (loop = 0; loop < half; loop++) {
        i = loop % N;
        j = loop / N;

        ki = zi = (4.0f * (i - (float)N/2)) / N;
        kj = zj = (4.0f * (j - (float)N/2)) / N;

        k = 1;
        while (((zi * zi) + (zj * zj) <= 4.0f) && (k++ < MAXITER)) { 
            new_zi = (zi * zi) - (zj * zj) + ki;
	        zj = 2.0f * zi * zj + kj;
	        zi = new_zi;
        }
	  
	    x[loop] = logf((float)k) / logf((float)MAXITER);
    }
    
/* ----------------------------------------------------------------*/
 
#ifdef FILE_IO
    printf("Writing mandelbrot.ppm\n");
    FILE *fp = fopen ("mandelbrot.ppm", "w");
    short  green, blue;
    fprintf (fp, "P6\n%4d %4d\n255\n", N, N);

    // Write top half
    for (loop = 0; loop < half; loop++) { 
	    if (x[loop] < 0.5) {
	        green = (int)(2 * x[loop] * 255);
            fprintf(fp, "%c%c%c", 255 - green, green, 0);
	    } else {
	        blue = (int)(2 * x[loop] * 255 - 255);
            fprintf(fp, "%c%c%c", 0, 255 - blue, blue);
	    }
    }
    // Write bottom half (top half mirrored)
    for (j = N/2 - 1; j >= 1; j--) {
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

    free(x);
}
