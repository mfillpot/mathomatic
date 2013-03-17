/*
 * Command-line numerical polynomial equation solver using the GNU Scientific Library (GSL).
 * GSL is available from: "http://www.gnu.org/software/gsl/".
 *
 * Copyright (C) 2008-2011 George Gesslein II
 
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

The chief copyright holder can be contacted at gesslein@mathomatic.org, or
George Gesslein II, P.O. Box 224, Lansing, NY  14882-0224  USA.
 
 */

/*
 * To compile:

./compile.roots

 * or

gcc -O3 -Wall -o roots roots.c -lgsl -lgslcblas

 * This program nicely solves any degree polynomial
 * with real coefficients by calling the
 * GSL function gsl_poly_complex_solve().
 * This file is also useful for testing
 * and as an example of using the GSL from C.
 * The results are double precision floating point values
 * that are sometimes accurate to 14 digits.
 * Complex numbers are output if successful.
 * Here is an example of it solving a cubic polynomial:

$ roots 1 1 1 1 
The 3 approximate floating point solutions of:
+x^3 +x^2 +x +1 = 0
are:

x = +0 +1*i
x = +0 -1*i
x = -1
$ 

Try this for a large amount of error:
roots -1 -1 1 1

Proof the GSL isn't perfect.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <gsl/gsl_poly.h>
#include <gsl/gsl_errno.h>

#define EPSILON	0.00000000000005	/* a good value for doubles */
#define HELP	1			/* display helpful text */

void	display_root(int i);

char	*prog_name = "roots";		/* name of this program */
double	*a, *z;				/* input and output arrays, respectively */
int	precision = DBL_DIG - 1;	/* display precision, it is not useful to set this higher than 15 */

void
usage(void)
{
  printf("\n%s version 1.0 - numerical polynomial equation solver\n", prog_name);
  printf("Uses the GNU Scientific Library (GSL).\n");
  printf("\nSolves polynomial = 0 when given all real coefficients of the polynomial.\n");
  printf("Double precision floating point math is used, accurate to about 14 digits.\n");
  printf("\nUsage: %s highest-power-coefficient ... constant-term\n", prog_name);
  printf("\nThe coefficients must be decimal, floating point, real numbers.\n");
  printf("For example, if 4 real numbers are given, there will be 3 complex number\n");
  printf("results or \"roots\" that are all valid solutions to polynomial = 0.\n");
}

int
main(int argc, char **argv)
{
  int i, highest_power;
  char *cp, *arg;
  gsl_poly_complex_workspace *w;

  if (argc <= 1) {
    fprintf(stderr, "%s: The polynomial coefficients must be specified on the command line.\n", prog_name);
    usage();
    exit(2);
  }

  highest_power = argc - 2;
  a = calloc(highest_power + 1, sizeof(double)); /* allocate real double input array */
  z = calloc(2 * highest_power, sizeof(double)); /* allocate complex double output array */

/* parse the command line into the coefficient array a[] */
  for (i = 0; i < argc - 1; i++) {
    arg = argv[argc-i-1];
    errno = 0;
    a[i] = strtod(arg, &cp);
    if (arg == cp || *cp) {
      fprintf(stderr, "%s: Argument \"%s\" is not a floating point number.\n", prog_name, arg);
      usage();
      exit(2);
    }
    if (errno) {
      fprintf(stderr, "%s: Argument \"%s\" is out of range.\n", prog_name, arg);
      exit(2);
    }
  }

#if	HELP
/* nicely display the actual polynomial equation we are solving */
  printf("The %d approximate floating point solutions of:\n", highest_power);
  for (i = highest_power; i >= 0; i--) {
    if (a[i]) {
      if (i && a[i] == 1.0) {
	printf("+x");
      } else {
        printf("%+.*g", precision, a[i]);
        if (i) {
	  printf("*x");
        }
      }
      if (i > 1) {
        printf("^%d", i);
      }
      printf(" ");
    }
  }
  printf("= 0\nare:\n\n");
#endif

/* solve the polynomial equation */
  w = gsl_poly_complex_workspace_alloc(highest_power + 1);
  if (gsl_poly_complex_solve(a, highest_power + 1, w, z) != GSL_SUCCESS) {
    fprintf(stderr, "%s: GSL approximation failed.\n", prog_name);
    exit(1);
  }
  gsl_poly_complex_workspace_free(w);

/* display all solutions */
  for (i = 0; i < highest_power; i++) {
#ifdef	EPSILON /* zero out relatively very small values (which are floating point error) */
    if (fabs(z[2*i] * EPSILON) > fabs(z[2*i+1]))
      z[2*i+1] = 0.0;
    else if (fabs(z[2*i+1] * EPSILON) > fabs(z[2*i]))
      z[2*i] = 0.0;
#endif
#if	HELP
    printf("x = ");
#endif
    display_root(i);
    printf("\n");
  }

  exit(0);
}

void
display_root(int i)
{
  printf("%+.*g", precision, z[2*i]);		/* output real part */
  if (z[2*i+1])
    printf(" %+.*g*i", precision, z[2*i+1]);	/* output imaginary part */
}
