/*
 * Tested long integer square root function.
 */

#include <stdio.h>
#include <stdlib.h>

#define	true	1
#define	false	0

#if	DEBUG
/*
 * Return true if x is the truncated integer square root of y.
 */
int
verify_lsqrt(long y, long x)
{
	if ((long long) x * x > y || ((long long) x + 1) * ((long long) x + 1) <= y) {
		return false;
	}
	return true;
}
#endif

/*
 * Returns the truncated integer square root of y using
 * the Babylonian iterative approximation method, derived from Newton's method.
 * Returns -1 on error.
 *
 * This lsqrt() function was written by George Gesslein II
 * and is placed in the public domain.
 */
long
lsqrt(long y)
{
	long	x_old, x_new;
	long	testy;
	int	nbits;
	int	i;

	if (y <= 0) {
		if (y != 0) {
#if	DEBUG
			fprintf(stderr, "Domain error in %s().\n", __func__);
#endif
			return -1L;
		}
		return 0L;
	}
/* select a good starting value using binary logarithms: */
	nbits = (sizeof(y) * 8) - 1;	/* subtract 1 for sign bit */
	for (i = 4, testy = 16L;; i += 2, testy <<= 2L) {
		if (i >= nbits || y <= testy) {
			x_old = (1L << (i / 2L));	/* x_old = sqrt(testy) */
			break;
		}
	}
/* x_old >= sqrt(y) */
/* use the Babylonian method to arrive at the integer square root: */
	for (;;) {
		x_new = (y / x_old + x_old) / 2L;
		if (x_old <= x_new)
			break;
		x_old = x_new;
	}
#if	DEBUG
	if (!verify_lsqrt(y, x_old)) {
		fprintf(stderr, "Verification error in %s().\n", __func__);
		return -1L;
	}
#endif
	return x_old;
}
