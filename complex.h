/*
 * Include file for the double precision floating point complex number
 * arithmetic functions in "complex_lib.c".
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

typedef struct complexs {	/* complex number structure */
	double	re;		/* real part */
	double	im;		/* imaginary part */
} complexs;

/*
 * Complex number arithmetic function prototypes
 */
int complex_fixup(complexs *ap);
complexs complex_add(complexs a, complexs b);
complexs complex_negate(complexs a);
complexs complex_mult(complexs a, complexs b);
complexs complex_div(complexs a, complexs b);
complexs complex_log(complexs a);
complexs complex_exp(complexs a);
complexs complex_pow(complexs a, complexs b);
