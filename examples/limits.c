/*
 * Standalone program to nicely display current C data type limits and sizes.
 *
 * Copyright (C) 2008-2012 George Gesslein II
 
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
 * Compile with:
 *
 * cc limits.c -o limits
 *
 * then type "./limits".
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

int
main(void)
{
  float		xf = 1.0;
  double	xd = 1.0;
#ifdef	LDBL_EPSILON
  long double	xl = 1.0;
#endif

#ifdef	__VERSION__
#ifdef	__GNUC__
  printf("GNU ");
#endif
  printf("C Compiler version: %s\n\n", __VERSION__);
#endif

  printf("C compiler integer limits:\n");
  printf("--------------------------\n");

  printf("CHAR_BIT: Bits of type char: %d\n", CHAR_BIT);
  printf("sizeof(char) = %u bytes.\n", (unsigned) sizeof(char));
  printf("sizeof(char *) = %u bytes.\n", (unsigned) sizeof(char *));

  if (CHAR_MIN < 0) {
    printf("Current type char is signed.\n");
  } else {
    printf("Current type char is unsigned.\n");
  }
  printf("CHAR_MAX: Maximum numeric value of type char: %d\n", CHAR_MAX);
  printf("CHAR_MIN: Minimum numeric value of type char: %d\n\n", CHAR_MIN);

  printf("SCHAR_MAX: Maximum value of type signed char: %d\n", SCHAR_MAX);
  printf("SCHAR_MIN: Minimum value of type signed char: %d\n\n", SCHAR_MIN);

  printf("UCHAR_MAX: Maximum value of type unsigned char: %u\n\n", (unsigned) UCHAR_MAX);

  printf("sizeof(short) = %u bytes.\n", (unsigned) sizeof(short));
  printf("SHRT_MAX: Maximum value of type short: %d\n", SHRT_MAX);
  printf("SHRT_MIN: Minimum value of type short: %d\n\n", SHRT_MIN);

  printf("USHRT_MAX: Maximum value of type unsigned short: %u\n\n", (unsigned) USHRT_MAX);

  printf("sizeof(int) = %u bytes.\n", (unsigned) sizeof(int));
  printf("INT_MAX: Maximum value of type int: %d\n", INT_MAX);
  printf("INT_MIN: Minimum value of type int: %d\n\n", INT_MIN);

  printf("UINT_MAX: Maximum value of type unsigned int: %u\n\n", UINT_MAX);

  printf("sizeof(long) = %u bytes.\n", (unsigned) sizeof(long));
  printf("LONG_MAX: Maximum value of type long: %ld\n", LONG_MAX);
  printf("LONG_MIN: Minimum value of type long: %ld\n\n", LONG_MIN);

  printf("ULONG_MAX: Maximum value of type unsigned long: %lu\n\n", ULONG_MAX);

#ifdef	ULLONG_MAX
  printf("sizeof(long long) = %u bytes.\n", (unsigned) sizeof(long long));
  printf("LLONG_MAX: Maximum value of type long long: %lld\n", LLONG_MAX);
  printf("LLONG_MIN: Minimum value of type long long: %lld\n\n", LLONG_MIN);

  printf("ULLONG_MAX: Maximum value of type unsigned long long: %llu\n\n", ULLONG_MAX);
#else
  printf("Type \"long long\" not supported by this C compiler.\n\n");
#endif

  printf("\nC compiler floating point limits:\n");
  printf("---------------------------------\n");

  printf("sizeof(float) = %u bytes.\n", (unsigned) sizeof(float));
  printf("FLT_DIG: Decimal digits of precision for float: %d\n", FLT_DIG);
  printf("sizeof(double) = %u bytes.\n", (unsigned) sizeof(double));
  printf("DBL_DIG: Decimal digits of precision for double: %d\n", DBL_DIG);
#ifdef	LDBL_DIG
  printf("sizeof(long double) = %u bytes.\n", (unsigned) sizeof(long double));
  printf("LDBL_DIG: Decimal digits of precision for long double: %d\n", LDBL_DIG);
#endif

  printf("\nFLT_MAX: Maximum value of float: %g\n", FLT_MAX);
  printf("FLT_MIN: Minimum value of float: %g\n\n", FLT_MIN);

  printf("DBL_MAX: Maximum value of double: %g\n", DBL_MAX);
  printf("DBL_MIN: Minimum value of double: %g\n\n", DBL_MIN);

#ifdef	LDBL_MAX
  printf("LDBL_MAX: Maximum value of long double: %Lg\n", LDBL_MAX);
  printf("LDBL_MIN: Minimum value of long double: %Lg\n\n", LDBL_MIN);
#else
  printf("Type \"long double\" not supported by this C compiler.\n\n");
#endif

  printf("Epsilon is the smallest x such that 1.0 + x != 1.0\n");
  printf("FLT_EPSILON: Float epsilon: %g", FLT_EPSILON);
  xf += (FLT_EPSILON / 2.0);
  if (xf == 1.0) {
    xf += FLT_EPSILON;
    if (xf > 1.0) {
      printf(" verified.");
    }
  }
  printf("\n");
  printf("DBL_EPSILON: Double epsilon: %g", DBL_EPSILON);
  xd += (DBL_EPSILON / 2.0);
  if (xd == 1.0) {
    xd += DBL_EPSILON;
    if (xd > 1.0) {
      printf(" verified.");
    }
  }
  printf("\n");
#ifdef	LDBL_EPSILON
  printf("LDBL_EPSILON: Long double epsilon: %Lg", LDBL_EPSILON);
  xl += (LDBL_EPSILON / 2.0);
  if (xl == 1.0) {
    xl += LDBL_EPSILON;
    if (xl > 1.0) {
      printf(" verified.");
    }
  }
  printf("\n");
#endif

  printf("\nEnd of limits program output.\n");

  return 0;
}
