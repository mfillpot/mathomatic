/*
 * Mathomatic floating point constant factorizing routines.
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

#include "includes.h"

static void try_factor(double arg);
static int fc_recurse(token_type *equation, int *np, int loc, int level, int level_code);

/* The following data is used to factor integers: */
static double nn, sqrt_value;
static double skip_multiples[] = {	/* Additive array that skips over multiples of 2, 3, 5, and 7. */
	10, 2, 4, 2, 4, 6, 2, 6,
	 4, 2, 4, 6, 6, 2, 6, 4,
	 2, 6, 4, 6, 8, 4, 2, 4,
	 2, 4, 8, 6, 4, 6, 2, 4,
	 6, 2, 6, 6, 4, 2, 4, 6,
	 2, 6, 4, 2, 4, 2,10, 2
};	/* sum of all numbers = 210 = (2*3*5*7) */

/*
 * Factor the integer in "value".
 * Store the prime factors in the unique[] array.
 *
 * Return true if successful.
 */
int
factor_one(value)
double	value;
{
	int	i;
	double	d;

	uno = 0;
	nn = value;
	if (nn == 0.0 || !isfinite(nn)) {
		/* zero or not finite */
		return false;
	}
	if (fabs(nn) >= MAX_K_INTEGER) {
		/* too large to factor */
		return false;
	}
	if (fmod(nn, 1.0) != 0.0) {
		/* not an integer */
		return false;
	}
	sqrt_value = 1.0 + sqrt(fabs(nn));
	try_factor(2.0);
	try_factor(3.0);
	try_factor(5.0);
	try_factor(7.0);
	d = 1.0;
	while (d <= sqrt_value) {
		for (i = 0; i < ARR_CNT(skip_multiples); i++) {
			d += skip_multiples[i];
			try_factor(d);
		}
	}
	if (nn != 1.0) {
		if (nn < 0 && nn != -1.0) {
			try_factor(fabs(nn));
		}
		try_factor(nn);
	}
	if (uno == 0) {
		try_factor(1.0);
	}
/* Do some floating point arithmetic self-checking.  If the following fails, it is due to a floating point bug. */
	if (nn != 1.0) {
		error_bug("Internal error factoring integers (final nn != 1.0).");
	}
	if (value != multiply_out_unique()) {
		error_bug("Internal error factoring integers (result array value is incorrect).");
	}
	return true;
}

/*
 * See if "arg" is one or more factors of "nn".
 * If so, save it and remove it from "nn".
 */
static void
try_factor(arg)
double	arg;
{
#if	DEBUG
	if (fmod(arg, 1.0) != 0.0) {
		error_bug("Trying factor that is not an integer!");
	}
#endif
	while (fmod(nn, arg) == 0.0) {
		if (uno > 0 && ucnt[uno-1] > 0 && unique[uno-1] == arg) {
			ucnt[uno-1]++;
		} else {
			while (uno > 0 && ucnt[uno-1] <= 0)
				uno--;
			unique[uno] = arg;
			ucnt[uno++] = 1;
		}
		nn /= arg;
#if	DEBUG
		if (fmod(nn, 1.0) != 0.0) {
			error_bug("nn turned non-integer in try_factor().");
		}
#endif
		sqrt_value = 1.0 + sqrt(fabs(nn));
		if (fabs(nn) <= 1.5 || fabs(arg) <= 1.5)
			break;
	}
}

/*
 * Convert unique[] back into the single integer it represents,
 * which was the value passed in the last call to factor_one(value).
 * Nothing is changed and the value is returned.
 */
double
multiply_out_unique(void)
{
	int	i, j;
	double	d;

	d = 1.0;
	for (i = 0; i < uno; i++) {
#if	DEBUG
		if (ucnt[i] < 0) {
			error_bug("Error in ucnt[] being negative.");
		}
#endif
		for (j = 0; j < ucnt[i]; j++) {
			d *= unique[i];
		}
	}
	return d;
}

/*
 * Display the integer prime factors in the unique[] array, even if mangled.
 * Must have had a successful call to factor_one(value) previously,
 * to fill out unique[] with the prime factors of value.
 *
 * Return true if successful.
 */
int
display_unique(void)
{
	int	i;
	double	value;

	if (uno <= 0)
		return false;
	value = multiply_out_unique();
	fprintf(gfp, "%.0f = ", value);
	for (i = 0; i < uno;) {
		if (ucnt[i] > 0) {
			fprintf(gfp, "%.0f", unique[i]);
		} else {
			i++;
			continue;
		}
		if (ucnt[i] > 1) {
			fprintf(gfp, "^%d", ucnt[i]);
		}
		do {
			i++;
		} while (i < uno && ucnt[i] <= 0);
		if (i < uno) {
			fprintf(gfp, " * ");
		}
	}
	fprintf(gfp, "\n");
	return true;
}

/*
 * Determine if the result of factor_one(x) is prime.
 *
 * Return true if x is a prime number.
 */
int
is_prime(void)
{
	double	value;

	if (uno <= 0) {
#if	DEBUG
		error_bug("uno == 0 in is_prime().");
#endif
		return false;
	}
	value = multiply_out_unique();
	if (value < 2.0)
		return false;
	if (uno == 1 && ucnt[0] == 1)
		return true;
	return false;
}

/*
 * Factor integers into their prime factors in an equation side.
 *
 * Return true if the equation side was modified.
 */
int
factor_int(equation, np)
token_type	*equation;
int		*np;
{
	int	i, j;
	int	xsize;
	int	level;
	int	modified = false;

	for (i = 0; i < *np; i += 2) {
		if (equation[i].kind == CONSTANT && factor_one(equation[i].token.constant) && uno > 0) {
			if (uno == 1 && ucnt[0] <= 1)
				continue;	/* prime number */
			level = equation[i].level;
			if (uno > 1 && *np > 1)
				level++;
			xsize = -2;
			for (j = 0; j < uno; j++) {
				if (ucnt[j] > 1)
					xsize += 4;
				else
					xsize += 2;
			}
			if (*np + xsize > n_tokens) {
				error_huge();
			}
			for (j = 0; j < uno; j++) {
				if (ucnt[j] > 1)
					xsize = 4;
				else
					xsize = 2;
				if (j == 0)
					xsize -= 2;
				if (xsize > 0) {
					blt(&equation[i+xsize], &equation[i], (*np - i) * sizeof(token_type));
					*np += xsize;
					if (j > 0) {
						i++;
						equation[i].kind = OPERATOR;
						equation[i].level = level;
						equation[i].token.operatr = TIMES;
						i++;
					}
				}
				equation[i].kind = CONSTANT;
				equation[i].level = level;
				equation[i].token.constant = unique[j];
				if (ucnt[j] > 1) {
					equation[i].level = level + 1;
					i++;
					equation[i].kind = OPERATOR;
					equation[i].level = level + 1;
					equation[i].token.operatr = POWER;
					i++;
					equation[i].level = level + 1;
					equation[i].kind = CONSTANT;
					equation[i].token.constant = ucnt[j];
				}
			}
			modified = true;
		}
	}
	return modified;
}

/*
 * Factor integers in an equation space.
 *
 * Return true if something was factored.
 */
int
factor_int_equation(n)
int	n;	/* equation space number */
{
	int	rv = false;

	if (empty_equation_space(n))
		return rv;
	if (factor_int(lhs[n], &n_lhs[n]))
		rv = true;
	if (factor_int(rhs[n], &n_rhs[n]))
		rv = true;
	return rv;
}

/*
 * List an equation side with optional integer factoring.
 */
int
list_factor(equation, np, factor_flag)
token_type	*equation;
int		*np;
int		factor_flag;
{
	if (factor_flag || factor_int_flag) {
		factor_int(equation, np);
	}
	return list_proc(equation, *np, false);
}

/*
 * Neatly factor out coefficients in additive expressions in an equation side.
 * For example: (2*x + 4*y + 6) becomes 2*(x + 2*y + 3).
 *
 * This routine is often necessary because the expression compare (se_compare())
 * does not return a multiplier (except for +/-1.0).
 * Normalization done here is required for simplification of algebraic fractions, etc.
 *
 * If "level_code" is 0, all additive expressions are normalized
 * by making at least one coefficient unity (1) by factoring out
 * the absolute value of the constant coefficient closest to zero.
 * This makes the absolute value of all other coefficients >= 1.
 * If all coefficients are negative, -1 will be factored out, too.
 *
 * If "level_code" is 1, any level 1 additive expression is factored
 * nicely for readability, while all deeper levels are normalized,
 * so that algebraic fractions are simplified.
 *
 * If "level_code" is 2, nothing is normalized unless it increases
 * readability.
 *
 * If "level_code" is 3, nothing is done.
 *
 * Add 4 to "level_code" to always factor out the GCD of rational coefficients
 * to produce all reduced integer coefficients.
 *
 * Return true if equation side was modified.
 */
int
factor_constants(equation, np, level_code)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to length of equation side */
int		level_code;	/* see above */
{
	if (level_code == 3)
		return false;
	return fc_recurse(equation, np, 0, 1, level_code);
}

static int
fc_recurse(equation, np, loc, level, level_code)
token_type	*equation;
int		*np, loc, level;
int		level_code;
{
	int	i, j, k, eloc;
	int	op;
	double	d, minimum = 1.0, cogcd = 1.0;
	int	improve_readability, gcd_flag, first = true, neg_flag = true, modified = false;
	int	op_count = 0, const_count = 0;

	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= fc_recurse(equation, np, i, level + 1, level_code);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	if (modified)
		return true;
	improve_readability = ((level_code & 3) > 1 || ((level_code & 3) && (level == 1)));
	gcd_flag = ((improve_readability && factor_out_all_numeric_gcds) || (level_code & 4));
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level == level) {
			switch (equation[i].kind) {
			case CONSTANT:
				const_count++;
				d = equation[i].token.constant;
				break;
			case OPERATOR:
				switch (equation[i].token.operatr) {
				case PLUS:
					neg_flag = false;
				case MINUS:
					op_count++;
					break;
				default:
					return modified;
				}
				i++;
				continue;
			default:
				d = 1.0;
				break;
			}
			if (i == loc && d > 0.0)
				neg_flag = false;
			d = fabs(d);
			if (first) {
				minimum = d;
				cogcd = d;
				first = false;
			} else {
				if (minimum > d)
					minimum = d;
				if (gcd_flag && cogcd != 0.0)
					cogcd = gcd_verified(d, cogcd);
			}
		} else {
			op = 0;
			for (j = i + 1; j < *np && equation[j].level > level; j += 2) {
#if	DEBUG
				if (equation[j].kind != OPERATOR) {
					error_bug("Bug in factor_constants().");
				}
#endif
				if (equation[j].level == level + 1) {
					op = equation[j].token.operatr;
				}
			}
			if (op == TIMES || op == DIVIDE) {
				for (k = i; k < j; k++) {
					if (equation[k].level == (level + 1) && equation[k].kind == CONSTANT) {
						if (i == j)
							return modified; /* more than one constant */
						if (k > i && equation[k-1].token.operatr != TIMES)
							return modified;
						d = equation[k].token.constant;
						if (i == loc && d > 0.0)
							neg_flag = false;
						d = fabs(d);
						if (first) {
							minimum = d;
							cogcd = d;
							first = false;
						} else {
							if (minimum > d)
								minimum = d;
							if (gcd_flag && cogcd != 0.0)
								cogcd = gcd_verified(d, cogcd);
						}
						i = j;
					}
				}
				if (i == j)
					continue;
			}
			if (i == loc)
				neg_flag = false;
			if (first) {
				minimum = 1.0;
				cogcd = 1.0;
				first = false;
			} else {
				if (minimum > 1.0)
					minimum = 1.0;
				if (gcd_flag && cogcd != 0.0)
					cogcd = gcd_verified(1.0, cogcd);
			}
			i = j;
			continue;
		}
		i++;
	}
	eloc = i;
	if (gcd_flag && cogcd != 0.0 /* && fmod(cogcd, 1.0) == 0.0 */) {
		minimum = cogcd;
	}
	if (first || op_count == 0 || const_count > 1 || (!neg_flag && minimum == 1.0))
		return modified;
	if (minimum == 0.0 || !isfinite(minimum))
		return modified;
	if (improve_readability) {
		for (i = loc; i < eloc;) {
			d = 1.0;
			if (equation[i].kind == CONSTANT) {
				if (equation[i].level == level || ((i + 1) < eloc
				    && equation[i].level == (level + 1)
				    && equation[i+1].level == (level + 1)
				    && (equation[i+1].token.operatr == TIMES
				    || equation[i+1].token.operatr == DIVIDE))) {
					d = equation[i].token.constant;
				}
			}
#if	0	/* was 1; changed to 0 for optimal results and so 180*(sides-2) simplification works nicely. */
			if (!gcd_flag && minimum >= 1.0) {
				minimum = 1.0;
				break;
			}
#endif
#if	1
			if ((minimum < 1.0) && (fmod(d, 1.0) == 0.0)) {
				minimum = 1.0;
				break;
			}
#endif
/* Make sure division by the number to factor out results in an integer: */
			if (fmod(d, minimum) != 0.0) {
				minimum = 1.0;	/* result not an integer */
				break;
			}
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			if (i >= *np || equation[i].level < level)
				break;
			i++;
		}
	}
	if (neg_flag)
		minimum = -minimum;
	if (minimum == 1.0)
		return modified;
	if (*np + ((op_count + 2) * 2) > n_tokens) {
		error_huge();
	}
	for (i = loc; i < *np && equation[i].level >= level; i++) {
		if (equation[i].kind != OPERATOR) {
			for (j = i;;) {
				equation[j].level++;
				j++;
				if (j >= *np || equation[j].level <= level)
					break;
			}
			blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
			*np += 2;
			equation[j].level = level + 1;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = DIVIDE;
			j++;
			equation[j].level = level + 1;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = minimum;
			i = j;
		}
	}
	for (i = loc; i < *np && equation[i].level >= level; i++) {
		equation[i].level++;
	}
	blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
	*np += 2;
	equation[i].level = level;
	equation[i].kind = OPERATOR;
	equation[i].token.operatr = TIMES;
	i++;
	equation[i].level = level;
	equation[i].kind = CONSTANT;
	equation[i].token.constant = minimum;
	return true;
}
