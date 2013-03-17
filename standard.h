/*
 * A standard include file for all math programs written in C.
 *
 * Copyright (C) 1987-2012 George Gesslein II.
 
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

#ifndef M_EULER
#define M_EULER	0.57721566490153286060651209008      /* Euler-Mascheroni constant (from GSL) */
#endif 
#ifndef	M_PI
#define M_PI	3.14159265358979323846	/* pi */
#endif
#ifndef	M_E
#define M_E	2.7182818284590452354	/* e */
#endif

#ifndef	max
#define max(a, b)    (((a) > (b)) ? (a) : (b))		/* return the maximum of two values */
#endif
#ifndef	min
#define min(a, b)    (((a) < (b)) ? (a) : (b))		/* return the minimum of two values */
#endif

#ifndef	INFINITY
#define INFINITY	HUGE_VAL			/* the floating point, positive infinity constant */
#endif

#ifndef	isfinite
#define	isfinite(d)	finite(d)			/* true if double d is finite (not infinity nor NaN) */
#endif

#ifndef	isinf
#if	sun						/* reconstruct missing isinf() function */
#define	isinf(d)	((fpclass(d) == FP_PINF) - (fpclass(d) == FP_NINF))
#endif
#endif

#define	ARR_CNT(a)	((int) (sizeof(a)/sizeof(a[0])))	/* returns the number of elements in an array */
#define CLEAR_ARRAY(a)	memset(a, 0, sizeof(a))			/* quickly sets all elements of an array to zero */
