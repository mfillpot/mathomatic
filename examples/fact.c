
#if	0
#define _REENTRANT      1       /* Should be defined before including math.h for Mac OS X. */
				/* Do not use with iOS.  Disabled by default and unnecessary. */
#endif

#include <math.h>

/*
 * General factorial function in C for double precision floating point.
 * Uses the threadsafe lgamma_r(3) function, if _REENTRANT is defined.
 * Works for any floating point value.
 * Recommend using tgamma(3) (true gamma) function instead, if available.
 *
 * Link with -lm
 *
 * Returns (arg!) (same as gamma(arg+1)).
 * Sets "errno" external variable on overflow or error.
 */
double
factorial(double arg)
{
	double	d;

#if	USE_TGAMMA
	d = tgamma(arg + 1.0);
	return d;
#else
#if	_REENTRANT
	int	sign;

	d = exp(lgamma_r(arg + 1.0, &sign));
	return(d * sign);
#else
	d = exp(lgamma(arg + 1.0)) * signgam;
	return d;
#endif
#endif
}
