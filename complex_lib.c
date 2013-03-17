/*
 * A handy, tested, small, stand-alone, double precision floating point
 * complex number arithmetic library for C.
 * Just include "complex.h" if you use this.
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

#include "complex.h"
#include <math.h>

#define	true	1
#define	false	0

#define epsilon	0.00000000000005	/* a good value for doubles */

/*
 * Zero out relatively very small real or imaginary parts of a complex number,
 * because they probably are a result of accumulated floating point inaccuracies.
 *
 * Return true if something was zeroed out.
 */
int
complex_fixup(ap)
complexs	*ap;	/* complex number pointer */
{
	if (fabs(ap->re * epsilon) > fabs(ap->im)) {
		ap->im = 0.0;
		return true;
	}
	if (fabs(ap->im * epsilon) > fabs(ap->re)) {
		ap->re = 0.0;
		return true;
	}
	return false;
}

/*
 * Add two complex numbers (a + b)
 * and return the complex number result.
 *
 * Complex number subtraction (a - b) is done by
 * complex_add(a, complex_negate(b)).
 */
complexs
complex_add(a, b)
complexs	a, b;
{
	a.re += b.re;
	a.im += b.im;
	return(a);
}

/*
 * Negate a complex number (-a)
 * and return the complex number result.
 */
complexs
complex_negate(a)
complexs	a;
{
	a.re = -a.re;
	a.im = -a.im;
	return(a);
}

/*
 * Multiply two complex numbers (a * b)
 * and return the complex number result.
 */
complexs
complex_mult(a, b)
complexs	a, b;
{
	complexs	r;

	r.re = a.re * b.re - a.im * b.im;
	r.im = a.re * b.im + a.im * b.re;
	return(r);
}

/*
 * Divide two complex numbers (a / b)
 * and return the complex number result.
 */
complexs
complex_div(a, b)
complexs	a;	/* dividend */
complexs	b;	/* divisor */
{
	complexs	r, num;
	double		denom;

	b.im = -b.im;
	num = complex_mult(a, b);
	denom = b.re * b.re + b.im * b.im;
	r.re = num.re / denom;
	r.im = num.im / denom;
	return r;
}

/*
 * Take the natural logarithm of a complex number
 * and return the complex number result.
 */
complexs
complex_log(a)
complexs	a;
{
	complexs	r;

	r.re = log(a.re * a.re + a.im * a.im) / 2.0;
	r.im = atan2(a.im, a.re);
	return(r);
}

/*
 * Raise the natural number (e) to the power of a complex number (e^a)
 * and return the complex number result.
 */
complexs
complex_exp(a)
complexs	a;
{
	complexs	r;
	double		m;

	m = exp(a.re);
	r.re = m * cos(a.im);
	r.im = m * sin(a.im);
	return(r);
}

/*
 * Raise complex number "a" to the power of complex number "b" (a^b)
 * and return the complex number result.
 */
complexs
complex_pow(a, b)
complexs	a, b;
{
	complexs	r;

	r = complex_log(a);
	r = complex_mult(r, b);
	r = complex_exp(r);
	complex_fixup(&r);
	return(r);
}
