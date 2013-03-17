/*
 * Mathomatic simplifying and general polynomial routines.
 * Includes polynomial and smart division, polynomial factoring, etc.
 * Globals tlhs[] and trhs[] are used and wiped out by most of these routines.
 *
 * The polynomial division and GCD routines here are not recursive,
 * due to the global static expression storage areas.
 * This limits the polynomial GCD routines to mostly univariate operation.
 * This also does not allow their use during solving.
 * So far, these limitations have been a good thing,
 * making Mathomatic faster and more stable and reliable.
 * Making polynomial GCD calculations partially recursive
 * with one level of recursion would enable multivariate operation for most cases, I think.
 *
 * Mathomatic has proved it is practical and efficient to do
 * generalized polynomial operations.  By generalized, I mean that
 * the coefficients of polynomials are not specially treated or stored in any way.
 * They are not separated out from the main variable of the polynomial.
 * Maximum simplification of all expressions is not possible unless everything is generalized.
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

#define	REMAINDER_IS_ZERO()	(n_trhs == 1 && trhs[0].kind == CONSTANT && trhs[0].token.constant == 0.0)

/*
 * The following static expression storage areas are of non-standard size
 * and must only be used for temporary storage.
 * Most Mathomatic expression manipulation and simplification routines should not be used
 * on non-standard or constant size expression storage areas.
 * Standard size expression storage areas that may be
 * manipulated or simplified are the equation spaces, tlhs[], trhs[], and tes[] only.
 */
token_type	divisor[DIVISOR_SIZE];		/* static expression storage areas for polynomial and smart division */
int		n_divisor;			/* length of expression in divisor[] */
token_type	quotient[DIVISOR_SIZE];
int		n_quotient;			/* length of expression in quotient[] */
token_type	gcd_divisor[DIVISOR_SIZE];	/* static expression storage area for polynomial GCD routine */
int		len_d;				/* length of expression in gcd_divisor[] */

static int pf_recurse(token_type *equation, int *np, int loc, int level, int do_repeat);
static int pf_sub(token_type *equation, int *np, int loc, int len, int level, int do_repeat);
static int save_factors(token_type *equation, int *np, int loc1, int len, int level);
static int do_gcd(long *vp);
static int mod_recurse(token_type *equation, int *np, int loc, int level);
static int polydiv_recurse(token_type *equation, int *np, int loc, int level);
static int pdiv_recurse(token_type *equation, int *np, int loc, int level, int code);
static int poly_div_sub(token_type *d1, int len1, token_type *d2, int len2, long *vp);
static int find_highest_count(token_type *p1, int n1, token_type *p2, int n2, long *vp1);

/*
 * Compare function for qsort(3).
 */
static int
vcmp(p1, p2)
sort_type	*p1, *p2;
{
	if (p2->count == p1->count) {
		if (p1->v < p2->v)
			return -1;
		if (p1->v == p2->v)
			return 0;
		return 1;
	}
	return(p2->count - p1->count);
}

/*
 * Return true if passed expression is strictly a single polynomial term in variable v.
 * The general form of a polynomial term is c*(v^d)
 * The coefficient (c) and exponent (d) may be any expression,
 * as long as it doesn't contain the variable v.
 */
int
poly_in_v_sub(p1, n, v, allow_divides)
token_type	*p1;		/* expression pointer */
int		n;		/* expression length */
long		v;		/* Mathomatic variable */
int		allow_divides;	/* if true, allow division by variable */
{
	int	i, k;
	int	level, vlevel;
	int	count;

	level = min_level(p1, n);
	for (i = 0, count = 0; i < n; i += 2) {
		if (p1[i].kind == VARIABLE && p1[i].token.variable == v) {
			count++;
			if (count > 1)
				return false;
			vlevel = p1[i].level;
			if (vlevel == level || vlevel == (level + 1)) {
				for (k = 1; k < n; k += 2) {
					if (p1[k].level == level) {
						switch (p1[k].token.operatr) {
						case DIVIDE:
							if (!allow_divides && k == (i - 1))
								return false;
						case TIMES:
							continue;
						case POWER:
							if (k == (i + 1))
								continue;
						default:
							return false;
						}
					}
				}
				if (vlevel == (level + 1)) {
					if ((i + 1) < n && p1[i+1].level == vlevel
					    && p1[i+1].token.operatr == POWER) {
						continue;
					}
				} else {
					continue;
				}
			}
			return false;
		}
	}
	return true;
}

/*
 * Return true if passed expression is a polynomial in the given variable v.
 * The coefficients and exponents may be anything, as long as they don't contain the variable v.
 * The passed expression should be fully unfactored, for a proper determination.
 */
int
poly_in_v(p1, n, v, allow_divides)
token_type	*p1;		/* expression pointer */
int		n;		/* expression length */
long		v;		/* Mathomatic variable */
int		allow_divides;	/* allow variable to be right of a divide (negative exponents) as a polynomial term */
{
	int	i, j;

	for (i = 1, j = 0;; i += 2) {
		if (i >= n || (p1[i].level == 1
		    && (p1[i].token.operatr == PLUS || p1[i].token.operatr == MINUS))) {
			if (!poly_in_v_sub(&p1[j], i - j, v, allow_divides)) {
				return false;
			}
			j = i + 1;
		}
		if (i >= n)
			break;
	}
	return true;
}

/*
 * Factor polynomials by calling pf_sub() for every additive sub-expression.
 * Factors repeated factor polynomials (like (x+1)^5) if do_repeat is true.
 * And always factors multivariate polynomials with symbolic factors (like (x+a)*(x+b)).
 *
 * Does not factor polynomials with different numeric factors (like (x+1)*(x+2)).
 *
 * Because of accumulated floating point round-off error,
 * this routine does not succeed at factoring most large polynomials.
 *
 * Return true if equation side was modified (factored).
 */
int
poly_factor(equation, np, do_repeat)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to length of equation side */
int		do_repeat;	/* factor repeated factors flag */
{
	return pf_recurse(equation, np, 0, 1, do_repeat);
}

static int
pf_recurse(equation, np, loc, level, do_repeat)
token_type	*equation;
int		*np, loc, level;
int		do_repeat;
{
	int	modified = false;
	int	i;
	int	count = 0, level_count = 0;

	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		switch (equation[i].token.operatr) {
		case PLUS:
		case MINUS:
			count++;
			if (equation[i].level == level) {
				level_count++;
			}
		}
	}
	if (level_count && count > 1) {	/* so we don't factor expressions with only one additive operator */
		/* try to factor the sub-expression */
		modified = pf_sub(equation, np, loc, i - loc, level, do_repeat);
	}
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= pf_recurse(equation, np, i, level + 1, do_repeat);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	return modified;
}

/*
 * Polynomial factoring subroutine.
 * It can't factor everything, but it usually can factor polynomials
 * if it will make the expression size smaller.
 *
 * Return true if equation side was modified (factored).
 */
static int
pf_sub(equation, np, loc, len, level, do_repeat)
token_type	*equation;	/* equation side holding the possible polynomial to factor */
int		*np,		/* pointer to length of equation side */
		loc,		/* index of start of polynomial in equation side */
		len;		/* length of polynomial */
int		level;		/* level of additive operators in polynomial */
int		do_repeat;	/* factor repeated factors flag */
{
	token_type	*p1;
	int		modified = false, symbolic_modified = false;
	int		i, j, k;
	long		v = 0, v1, last_v;
	int		len_first = 0;
	int		loc1, loc2, len2 = 0;
	int		loct, lent;
	int		count;
	jmp_buf		save_save;
	int		div_flag = 3;
	int		vc, cnt;
	sort_type	va[MAX_VARS];
	double		d;
	int		old_partial;

	debug_string(3, "Entering pf_sub().");
	old_partial = partial_flag;
	loc2 = loc1 = loc;
	find_greatest_power(&equation[loc1], len, &v, &d, &j, &k, &div_flag);
	if (v == 0)
		return false;
	blt(save_save, jmp_save, sizeof(jmp_save));
	if ((i = setjmp(jmp_save)) != 0) {	/* trap errors */
		partial_flag = old_partial;
		blt(jmp_save, save_save, sizeof(jmp_save));
		if (i == 13) {	/* critical error code */
			longjmp(jmp_save, i);
		}
		return(modified || symbolic_modified);
	}
/* First factor polynomials with repeated factors */
/* using poly_gcd(polynomial, v * differentiate(polynomial, v)) to discover the factors: */
	for (count = 1; do_repeat; count++) {
		blt(trhs, &equation[loc1], len * sizeof(token_type));
		n_trhs = len;
		partial_flag = false;
		uf_simp(trhs, &n_trhs);
		partial_flag = old_partial;
		if (level1_plus_count(trhs, n_trhs) < 2) {
/* must be at least 2 level 1 additive operators to be factorable */
			goto skip_factor;
		}
/* create a variable list with counts of the number of times each variable occurs: */
		last_v = 0;
		for (vc = 0; vc < ARR_CNT(va);) {
			cnt = 0;
			v1 = -1;
			for (i = 0; i < n_trhs; i += 2) {
				if (trhs[i].kind == VARIABLE && trhs[i].token.variable > last_v) {
					if (v1 == -1 || trhs[i].token.variable < v1) {
						v1 = trhs[i].token.variable;
						cnt = 1;
					} else if (trhs[i].token.variable == v1) {
						cnt++;
					}
				}
			}
			if (v1 == -1)
				break;
			last_v = v1;
			va[vc].v = v1;
			va[vc].count = cnt;
			vc++;
		}
		side_debug(3, &equation[loc1], len);
		side_debug(3, trhs, n_trhs);
/* Find a valid polynomial base variable "v": */
		cnt = -1;
		if (v) {
			if (vc > 1 && !poly_in_v(trhs, n_trhs, v, true)) {
				v = 0;
			}
		}
		for (i = 0; i < vc; i++) {
			if ((va[i].v & VAR_MASK) <= SIGN) {
				continue;
			}
			if (v == 0) {
				if (poly_in_v(trhs, n_trhs, va[i].v, true)) {
					v = va[i].v;
				}
			}
			if (cnt < 0 || va[i].count < cnt) {
				cnt = va[i].count;
			}
		}
		if (cnt <= 1)	/* A polynomial requires 2 or more instances of every normal variable to be factorable. */
			goto skip_factor;
		if (v == 0) {
#if	1
			goto skip_factor;
#else
/* no polynomial variables found, try differentiating with respect to each normal variable until one works */
			for (i = 0; i < vc; i++) {
				if ((va[i].v & VAR_MASK) <= SIGN) {
					continue;
				}
				v = va[i].v;
				blt(tlhs, trhs, n_trhs * sizeof(token_type));
				n_tlhs = n_trhs;
				if (differentiate(tlhs, &n_tlhs, v)) {
					break;
				}
				v = 0;
			}
			if (v == 0) {
				goto skip_factor;
			}
#endif
		} else {
			blt(tlhs, trhs, n_trhs * sizeof(token_type));
			n_tlhs = n_trhs;
			if (!differentiate(tlhs, &n_tlhs, v)) {
				break;
			}
		}
#if	!SILENT
		if (debug_level >= 3) {
			list_var(v, 0);
			fprintf(gfp, _("Differentiation successful using variable %s.\n"), var_str);
		}
#endif
		simp_loop(tlhs, &n_tlhs);
		if ((n_tlhs + 2) > min(DIVISOR_SIZE, n_tokens))
			break;
		for (i = 0; i < n_tlhs; i++)
			tlhs[i].level++;
		tlhs[n_tlhs].kind = OPERATOR;
		tlhs[n_tlhs].level = 1;
		tlhs[n_tlhs].token.operatr = TIMES;
		n_tlhs++;
		tlhs[n_tlhs].kind = VARIABLE;
		tlhs[n_tlhs].level = 1;
		tlhs[n_tlhs].token.variable = v;
		n_tlhs++;
		uf_simp(tlhs, &n_tlhs);
		if (poly_gcd(&equation[loc1], len, tlhs, n_tlhs, v) <= 0)
			break;
		if (level1_plus_count(tlhs, n_tlhs) == 0)
			break;
		if (!save_factors(equation, np, loc1, len, level))
			break;
		loc1 += n_tlhs + 1;
		len = n_trhs;
		switch (count) {
		case 1:
			debug_string(1, "Polynomial with repeated factor factored.");
			len_first = n_tlhs;
			loc2 = loc1;
			break;
		case 2:
			len2 = n_tlhs;
			break;
		}
		modified = true;
	}
/* Now factor polynomials with symbolic factors by grouping: */
	if (!modified) {
		last_v = 0;
next_v:
		p1 = &equation[loc1];
		blt(trhs, p1, len * sizeof(token_type));
		n_trhs = len;
		uf_simp_no_repeat(trhs, &n_trhs);
		if (level1_plus_count(trhs, n_trhs) < 2) {
/* must be at least 2 level 1 additive operators to be factorable */
			goto skip_factor;
		}
		for (;;) {
			v = -1;
			for (i = 0; i < len; i += 2) {
				if (p1[i].kind == VARIABLE && p1[i].token.variable > last_v) {
					if (v == -1 || p1[i].token.variable < v) {
						v = p1[i].token.variable;
					}
				}
			}
			if (v == -1) {
				break;
			}
			last_v = v;
			/* make sure there is more than one "v" raised to the highest power: */
			if (find_greatest_power(trhs, n_trhs, &v, &d, &j, &k, &div_flag) <= 1) {
				continue;
			}
			blt(tlhs, trhs, n_trhs * sizeof(token_type));
			n_tlhs = n_trhs;
			/* do the grouping: */
			while (factor_plus(tlhs, &n_tlhs, v, 0.0)) {
				simp_loop(tlhs, &n_tlhs);
			}
			/* extract the highest power group: */
			if (find_greatest_power(tlhs, n_tlhs, &v, &d, &j, &k, &div_flag) != 1) {
				continue;
			}
			if (j) {
				blt(tlhs, &tlhs[j], k * sizeof(token_type));
			}
			n_tlhs = k;
#if	!SILENT
			if (debug_level >= 3) {
				fprintf(gfp, _("Trying factor: "));
				list_proc(tlhs, n_tlhs, false);
				fprintf(gfp, "\n");
			}
#endif
			if (poly_gcd(&equation[loc1], len, tlhs, n_tlhs, 0L) <= 0)
				goto next_v;
			if (level1_plus_count(tlhs, n_tlhs) == 0)
				goto next_v;
			if (!symbolic_modified) {
				debug_string(1, "Symbolic polynomial factored.");
			} else {
				debug_string(1, "Found another symbolic factor.");
			}
			if (!save_factors(equation, np, loc1, len, level))
				break;
			len = n_tlhs;
			symbolic_modified = true;
			last_v = 0;
			goto next_v;
		}
	}
skip_factor:
	blt(jmp_save, save_save, sizeof(jmp_save));
	if (modified) {
/* Repeated factor was factored out. */
/* See if we can factor out more of the repeated factor. */
		if (len2) {
			loct = loc2;
			lent = len2;
		} else {
			loct = loc;
			lent = len_first;
		}
		if (poly_gcd(&equation[loc1], len, &equation[loct], lent, v) > 0) {
			if (save_factors(equation, np, loc1, len, level)) {
				loc1 += n_tlhs + 1;
				len = n_trhs;
			}
		}
		if (len2) {
			loc1 = loc2;
			len = len2;
		}
		if (poly_gcd(&equation[loc], len_first, &equation[loc1], len, 0L) > 0) {
			save_factors(equation, np, loc, len_first, level);
		}
	}
	if (modified || symbolic_modified) {
		for (i = loc; i < *np && equation[i].level >= level; i++)
			;
#if	DEBUG
		if ((i & 1) != 1) {
			error_bug("Error in result of pf_sub().");
		}
#endif
		debug_string(1, "Resulting factors of pf_sub():");
		side_debug(1, &equation[loc], i - loc);
	}
	return(modified || symbolic_modified);
}

static int
save_factors(equation, np, loc1, len, level)
token_type	*equation;
int		*np, loc1, len, level;
{
	int	i, j;

	i = n_tlhs + 1 + n_trhs;
	if (i > (len * 3))
		goto rejected;
	if ((*np + (i - len)) > n_tokens)
		goto rejected;
	blt(&equation[loc1+i], &equation[loc1+len], (*np - (loc1 + len)) * sizeof(token_type));
	*np += i - len;
	blt(&equation[loc1], tlhs, n_tlhs * sizeof(token_type));
	i = loc1 + n_tlhs;
	equation[i].level = 0;
	equation[i].kind = OPERATOR;
	equation[i].token.operatr = TIMES;
	i++;
	blt(&equation[i], trhs, n_trhs * sizeof(token_type));
	i += n_trhs;
	for (j = loc1; j < i; j++)
		equation[j].level += level;
	return true;
rejected:
	debug_string(1, "Polynomial factor rejected because too large.");
	return false;
}

/*
 * Remove level 1 trivial factors and divides from tlhs[].
 * Makes it a simpler factor that looks a lot nicer and is easier to work with.
 *
 * Return true if result is a level 1 additive expression.
 * If this returns false, nothing is removed.
 */
int
remove_factors(void)
{
	int	i, j, k;
	int	plus_flag = false, divide_flag = false;
	int	op;

	debug_string(3, "Entering remove_factors() with: ");
	side_debug(3, tlhs, n_tlhs);
	do {
		simp_ssub(tlhs, &n_tlhs, 0L, 1.0, false, true, 4);
	} while (uf_power(tlhs, &n_tlhs));
	for (i = 1, j = 0, k = 0;; i += 2) {
		if (i >= n_tlhs) {
			if (plus_flag && !divide_flag) {
				if (k > 0)
					j--;
				blt(&scratch[k], &tlhs[j], (i - j) * sizeof(token_type));
				k += i - j;
			}
			if (k <= 0) {
				debug_string(3, "Leaving remove_factors() with false return and no change.");
				return false;
			}
			blt(tlhs, scratch, k * sizeof(token_type));
			n_tlhs = k;
			debug_string(3, "Leaving remove_factors() with success and: ");
			side_debug(3, tlhs, n_tlhs);
			return true;
		}
		op = tlhs[i].token.operatr;
		switch (tlhs[i].level) {
		case 1:
			switch (op) {
			case PLUS:
			case MINUS:
				plus_flag = true;
				continue;
			case TIMES:
			case DIVIDE:
				break;
			default:
				debug_string(3, "Leaving remove_factors() with false return and no change.");
				return false;
			}
			if (plus_flag && !divide_flag) {
				if (k > 0)
					j--;
				blt(&scratch[k], &tlhs[j], (i - j) * sizeof(token_type));
				k += i - j;
			}
			plus_flag = false;
			divide_flag = (op == DIVIDE);
			j = i + 1;
			break;
		case 2:
			switch (op) {
			case PLUS:
			case MINUS:
				plus_flag = true;
			}
			break;
		}
	}
}

/*
 * This is the Euclidean GCD algorithm applied to polynomials.
 * It needs to be made multivariate by making it recursive.
 *
 * Return the number of iterations (divisions), if successful,
 * with the polynomial GCD result in gcd_divisor[] and len_d.
 * Else return 0 or negative on failure.  If 0, might work if operands switched,
 * If a negative number, switching operands won't work either.
 */
static int
do_gcd(vp)
long		*vp;	/* polynomial base variable pointer */
{
	int	i;
	int	count;

	for (count = 1; count < 50; count++) {
		switch (poly_div(trhs, n_trhs, gcd_divisor, len_d, vp)) {
		case 0:
			/* divide failed */
			return(1 - count);
		case 2:
			/* Total success!  Remainder is zero. */
			debug_string(2, "Found raw polynomial GCD:");
			side_debug(2, gcd_divisor, len_d);
			return count;
		}
		/* Do the Euclidean shuffle: swap trhs[] (remainder) and gcd_divisor[] */
		if (len_d > n_tokens || n_trhs > DIVISOR_SIZE)
			return 0;
		blt(scratch, trhs, n_trhs * sizeof(token_type));
		blt(trhs, gcd_divisor, len_d * sizeof(token_type));
		blt(gcd_divisor, scratch, n_trhs * sizeof(token_type));
		i = n_trhs;
		n_trhs = len_d;
		len_d = i;
	}
	return 0;
}

/*
 * Compute the simplified and normalized polynomial Greatest Common Divisor
 * of the expressions in "larger" and "smaller".
 * This polynomial GCD routine is used for polynomial factoring, etc.
 *
 * Return a positive integer if successful.
 * Return the GCD in trhs[].
 * Return larger/GCD in tlhs[].
 * The results are unfactored and simplified.
 */
int
poly_gcd(larger, llen, smaller, slen, v)
token_type	*larger;	/* larger polynomial */
int		llen;		/* larger polynomial length */
token_type	*smaller;	/* smaller polynomial */
int		slen;		/* smaller polynomial length */
long		v;		/* polynomial base variable */
{
	int		count;

	debug_string(3, "Entering poly_gcd():");
	side_debug(3, larger, llen);
	side_debug(3, smaller, slen);
	if (llen > n_tokens || slen > min(ARR_CNT(gcd_divisor), n_tokens))
		return 0;
	if (trhs != larger) {
		blt(trhs, larger, llen * sizeof(token_type));
	}
	n_trhs = llen;
	if (tlhs != smaller) {
		blt(tlhs, smaller, slen * sizeof(token_type));
	}
	n_tlhs = slen;
	if (!remove_factors())
		return 0;
	if (n_tlhs > ARR_CNT(gcd_divisor))
		return 0;
	blt(gcd_divisor, tlhs, n_tlhs * sizeof(token_type));
	len_d = n_tlhs;
	count = do_gcd(&v);
	if (count <= 0)
		return 0;
	if (count > 1) {
		if (len_d > n_tokens)
			return 0;
		blt(tlhs, gcd_divisor, len_d * sizeof(token_type));
		n_tlhs = len_d;
		if (!remove_factors())
			return 0;
		if (n_tlhs > ARR_CNT(gcd_divisor))
			return 0;
		blt(gcd_divisor, tlhs, n_tlhs * sizeof(token_type));
		len_d = n_tlhs;
		if (poly_div(larger, llen, gcd_divisor, len_d, &v) != 2) {
			debug_string(1, "Polynomial GCD found, but larger divide failed in poly_gcd().");
			return 0;
		}
	}
	if (len_d > n_tokens)
		return 0;
	blt(trhs, gcd_divisor, len_d * sizeof(token_type));
	n_trhs = len_d;
	uf_simp(tlhs, &n_tlhs);
	uf_simp(trhs, &n_trhs);
	debug_string(3, "poly_gcd() successful.");
	return(count);
}

/*
 * Compute the polynomial Greatest Common Divisor of the expressions in "larger" and "smaller".
 * This polynomial GCD routine is used by the division simplifiers.
 *
 * Return a positive integer if successful.
 * Return larger/GCD in tlhs[].
 * Return smaller/GCD in trhs[].
 */
int
poly2_gcd(larger, llen, smaller, slen, v, require_additive)
token_type	*larger;	/* larger polynomial */
int		llen;		/* larger polynomial length */
token_type	*smaller;	/* smaller polynomial */
int		slen;		/* smaller polynomial length */
long		v;		/* polynomial base variable */
int		require_additive;	/* require the GCD to contain addition or subtraction */
{
	int		i;
	int		count;
#if	0
	jmp_buf		save_save;
#endif

	if (require_additive) {
/* Require an additive operator in both polynomials to continue. */
		count = 0;
		for (i = 1; i < llen; i += 2) {
			if (larger[i].token.operatr == PLUS || larger[i].token.operatr == MINUS) {
				count++;
				break;
			}
		}
		if (count == 0)
			return 0;
		count = 0;
		for (i = 1; i < slen; i += 2) {
			if (smaller[i].token.operatr == PLUS || smaller[i].token.operatr == MINUS) {
				count++;
			}
		}
		if (count == 0 /* || count > 200 */)
			return 0;
	}
	debug_string(3, "Entering poly2_gcd():");
	side_debug(3, larger, llen);
	side_debug(3, smaller, slen);
	if (llen > n_tokens || slen > min(ARR_CNT(gcd_divisor), n_tokens))
		return 0;
	blt(trhs, larger, llen * sizeof(token_type));
	n_trhs = llen;
	blt(tlhs, smaller, slen * sizeof(token_type));
	n_tlhs = slen;
#if	0
	if (require_additive) {
/* Require a level 1 additive operator in the divisor. */
		blt(save_save, jmp_save, sizeof(jmp_save));
		if ((i = setjmp(jmp_save)) != 0) {	/* trap errors */
			blt(jmp_save, save_save, sizeof(jmp_save));
			if (i == 13) {	/* critical error code */
				longjmp(jmp_save, i);
			}
			return 0;
		}
		uf_simp(tlhs, &n_tlhs);
		blt(jmp_save, save_save, sizeof(jmp_save));
		if (level1_plus_count(tlhs, n_tlhs) == 0)
			return 0;
	}
#endif
	if (n_tlhs > ARR_CNT(gcd_divisor))
		return 0;
	blt(gcd_divisor, tlhs, n_tlhs * sizeof(token_type));
	len_d = n_tlhs;
	count = do_gcd(&v);
	if (count <= 0)
		return count;
	if (count > 1) {
		if (require_additive && level1_plus_count(gcd_divisor, len_d) == 0)
			return 0;
		if (poly_div(smaller, slen, gcd_divisor, len_d, &v) != 2) {
			debug_string(1, "Polynomial GCD found, but smaller divide failed in poly2_gcd().");
			return 0;
		}
		blt(trhs, gcd_divisor, len_d * sizeof(token_type));
		n_trhs = len_d;
		if (n_tlhs > ARR_CNT(gcd_divisor))
			return 0;
		blt(gcd_divisor, tlhs, n_tlhs * sizeof(token_type));
		len_d = n_tlhs;
		blt(tlhs, trhs, n_trhs * sizeof(token_type));
		n_tlhs = n_trhs;
		if (poly_div(larger, llen, tlhs, n_tlhs, &v) != 2) {
			debug_string(1, "Polynomial GCD found, but larger divide failed in poly2_gcd().");
			return 0;
		}
		blt(trhs, gcd_divisor, len_d * sizeof(token_type));
		n_trhs = len_d;
	} else {
		n_trhs = 1;
		trhs[0] = one_token;
	}
	debug_string(3, "poly2_gcd() successful.");
	return count;
}

/*
 * This function returns true if the passed Mathomatic variable
 * is of type integer.
 *
 * Integer variable names start with "integer".
 */
int
is_integer_var(v)
long	v;
{
	char	*cp;
	int	(*strncmpfunc)();

	if (case_sensitive_flag) {
		strncmpfunc = strncmp;
	} else {
		strncmpfunc = strncasecmp;
	}

	cp = var_name(v);
	if (cp && strncmpfunc(cp, V_INTEGER_PREFIX, strlen(V_INTEGER_PREFIX)) == 0)
		return true;
	else
		return false;
}

/*
 * This function is a strict test that
 * returns true if passed expression is entirely integer and all variables
 * are either "integer" or "sign".
 *
 * The result of evaluating the expression must be integer if this returns true.
 * Should first be unfactored with uf_pplus() for a proper determination.
 */
int
is_integer_expr(p1, n)
token_type	*p1;	/* expression pointer */
int		n;	/* length of expression */
{
	int	i;
	long	v;

#if	DEBUG
	if (p1 == NULL || n < 1) {
		error_bug("(p1 == NULL || n < 1) in is_integer_expr().");
	}
#endif
	for (i = 0; i < n; i++) {
		switch (p1[i].kind) {
		case OPERATOR:
			if (p1[i].token.operatr == DIVIDE)
				return false;
			break;
		case CONSTANT:
			if (fmod(p1[i].token.constant, 1.0) != 0.0)
				return false;
			break;
		case VARIABLE:
			v = labs(p1[i].token.variable);
			if (!is_integer_var(v) && (v & VAR_MASK) != SIGN)
				return false;
			break;
		}
	}
	return true;
}

/*
 * This routine is the modulus operator (%) simplifier for equation sides.
 * Using "integer" variables will allow more simplification here.
 * Globals tlhs[] and trhs[] are wiped out by the polynomial division.
 *
 * Return true if equation side was modified.
 */
int
mod_simp(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of the equation side */
{
	return mod_recurse(equation, np, 0, 1);
}

static int
mod_recurse(equation, np, loc, level)
token_type	*equation;
int		*np, loc, level;
{
	int	modified = false;
	int	i, j, k;
	int	i1, i2, i3, i4, i5;
	int	op, last_op2;
	int	len1, len2, len3;
	int	diff_sign;

	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= mod_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	if (modified)	/* make sure the deepest levels are completed, first */
		return true;
	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		if (!(equation[i].level == level && equation[i].token.operatr == MODULUS))
			continue;
		for (k = i + 2;; k += 2) {
			if (k >= *np || equation[k].level <= level)
				break;
		}
		len1 = k - (i + 1);
		last_op2 = 0;
		for (j = loc; j < *np && equation[j].level >= level; j++) {
			if (equation[j].level == level && equation[j].kind == OPERATOR) {
				last_op2 = equation[j].token.operatr;
				continue;
			}
			if (last_op2 == MODULUS) {
				continue;
			}
			last_op2 = MODULUS;
			op = 0;
			for (i1 = k = j + 1; k < *np && equation[k].level > level; k += 2) {
				if (equation[k].level == (level + 1)) {
					op = equation[k].token.operatr;
					i1 = k;
				}
			}
			len2 = k - j;
			switch (op) {
			case MODULUS:
				/* simplify (x%n)%n to x%n */
				len3 = k - (i1 + 1);
				if (se_compare(&equation[i+1], len1, &equation[i1+1], len3, &diff_sign)) {
					blt(&equation[i1], &equation[k], (*np - k) * sizeof(token_type));
					*np -= len3 + 1;
					return true;
				}
				break;
			case TIMES:
				if (!is_integer_expr(&equation[j], len2))
					break;
				/* simplify (i%n*j)%n to (i*j)%n if j is integer */
				for (i2 = i1 = j + 1;; i1 += 2) {
					if (i1 >= k || equation[i1].level == (level + 1)) {
						for (; i2 < i1; i2 += 2) {
							if (equation[i2].level == (level + 2)
							    && equation[i2].token.operatr == MODULUS) {
								len3 = i1 - (i2 + 1);
								if (se_compare(&equation[i+1], len1, &equation[i2+1], len3, &diff_sign)) {
									blt(&equation[i2], &equation[i1], (*np - i1) * sizeof(token_type));
									*np -= len3 + 1;
									return true;
								}
							}
						}
					}
					if (i1 >= k)
						break;
				}
				break;
			case PLUS:
			case MINUS:
				/* simplify (i%n+j)%n to (i+j)%n */
				/* and ((i%n)*j+k)%n to (i*j+k)%n, when j is integer */
				for (i2 = i1 = j + 1, i3 = j - 1;; i1 += 2) {
					if (i1 >= k || equation[i1].level == (level + 1)) {
						for (; i2 < i1; i2 += 2) {
							if (equation[i2].level == (level + 2)) {
								switch (equation[i2].token.operatr) {
								case MODULUS:
									len3 = i1 - (i2 + 1);
									if (se_compare(&equation[i+1], len1, &equation[i2+1], len3, &diff_sign)) {
										blt(&equation[i2], &equation[i1], (*np - i1) * sizeof(token_type));
										*np -= len3 + 1;
										return true;
									}
									break;
								case TIMES:
									i2 = i1 - 2;
									if (!is_integer_expr(&equation[i3+1], i1 - (i3 + 1))) {
										break;
									}
									for (i4 = i3 + 2; i4 < i1; i4 += 2) {
										if (equation[i4].level == (level + 3)
										    && equation[i4].token.operatr == MODULUS) {
											for (i5 = i4 + 2; i5 < i1 && equation[i5].level > (level + 3); i5 += 2)
												;
											len3 = i5 - (i4 + 1);
											if (se_compare(&equation[i+1], len1, &equation[i4+1], len3, &diff_sign)) {
												blt(&equation[i4], &equation[i5], (*np - i5) * sizeof(token_type));
												*np -= len3 + 1;
												return true;
											}
										}
									}
									break;
								}
							}
						}
						i3 = i1;
					}
					if (i1 >= k)
						break;
				}
				break;
			}
#if	1
			/* Remove integer*n multiples in x for x%n by doing */
			/* polynomial division x/n and replacing with remainder%n. */
			/* Globals tlhs[] and trhs[] are wiped out by the polynomial division here. */
			if (poly_div(&equation[j], len2, &equation[i+1], len1, NULL)) {
				uf_pplus(tlhs, &n_tlhs);	/* so integer%(integer^integer) isn't simplified to 0 */
				if (is_integer_expr(tlhs, n_tlhs)) {
					if (n_trhs < len2 || REMAINDER_IS_ZERO()) {	/* if it will make it smaller */
						if ((*np + (n_trhs - len2)) > n_tokens)
							error_huge();
						for (k = 0; k < n_trhs; k++)
							trhs[k].level += level;
						blt(&equation[j+n_trhs], &equation[j+len2], (*np - (j + len2)) * sizeof(token_type));
						*np += n_trhs - len2;
						blt(&equation[j], trhs, n_trhs * sizeof(token_type));
						debug_string(2, "Polynomial division successful in modulus simplification.  The result is:");
						side_debug(2, equation, *np);
						return true;
					}
				}
			}
#endif
		}
	}
	return modified;
}

/*
 * This routine is a division simplifier for equation sides.
 * It reduces algebraic fractions.
 *
 * Return true if any polynomial Greatest Common Divisor (GCD)
 * between a numerator and denominator was found and
 * the expression was simplified by dividing the numerator
 * and denominator by the GCD.
 */
int
poly_gcd_simp(equation, np)
token_type	*equation;
int		*np;
{
	return polydiv_recurse(equation, np, 0, 1);
}

static int
polydiv_recurse(equation, np, loc, level)
token_type	*equation;
int		*np, loc, level;
{
	int	modified = false;
	int	i, j, k;
	int	last_op2;
	int	len1, len2;
	int	rv;

	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= polydiv_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
start:
	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
#if	DEBUG
		if (equation[i].kind != OPERATOR)
			error_bug("Bug in poly_gcd_simp().");
#endif
		if (equation[i].level == level && equation[i].token.operatr == DIVIDE) {
			for (k = i + 2;; k += 2) {
				if (k >= *np || equation[k].level <= level)
					break;
			}
			len1 = k - (i + 1);
			last_op2 = 0;
			for (j = loc; j < *np && equation[j].level >= level; j++) {
				if (equation[j].level == level && equation[j].kind == OPERATOR) {
					last_op2 = equation[j].token.operatr;
					continue;
				}
				switch (last_op2) {
				case DIVIDE:
					continue;
				case 0:
				case TIMES:
					break;
				default:
					error_bug("Expression is corrupt in poly_gcd_simp().");
				}
				last_op2 = DIVIDE;
				for (k = j + 1;; k += 2) {
					if (k >= *np || equation[k].level <= level)
						break;
				}
				len2 = k - j;
				if ((rv = poly2_gcd(&equation[i+1], len1, &equation[j], len2, 0L, true)) > 0) {
store_code:
					for (k = 0; k < n_tlhs; k++)
						tlhs[k].level += level;
					for (k = 0; k < n_trhs; k++)
						trhs[k].level += level;
					if (((*np + (n_trhs - len2)) > n_tokens)
					    || ((*np + (n_trhs - len2) + (n_tlhs - len1)) > n_tokens))
						error_huge();
					blt(&equation[j+n_trhs], &equation[j+len2], (*np - (j + len2)) * sizeof(token_type));
					*np += n_trhs - len2;
					if (i > j)
						i += n_trhs - len2;
					blt(&equation[j], trhs, n_trhs * sizeof(token_type));
					blt(&equation[i+n_tlhs+1], &equation[i+1+len1], (*np - (i + 1 + len1)) * sizeof(token_type));
					*np += n_tlhs - len1;
					blt(&equation[i+1], tlhs, n_tlhs * sizeof(token_type));
					debug_string(1, _("Division simplified with polynomial GCD."));
					modified = true;
					goto start;
				}
				if (rv == 0 && poly2_gcd(&equation[j], len2, &equation[i+1], len1, 0L, true) > 0) {
					k = j - 1;
					j = i + 1;
					i = k;
					k = len1;
					len1 = len2;
					len2 = k;
					goto store_code;
				}
			}
		}
	}
	return modified;
}

/*
 * This routine is a division simplifier for equation sides.
 * Check for divides and do polynomial and smart division.
 * For example: (c - b + a)/(b - a) will be converted to (c/(b - a)) - 1.
 * Will only make expressions with algebraic fractions smaller,
 * otherwise it would go into an infinite loop.
 * An expression with less operators is judged to be smaller.
 * Complex fractions (fractions within fractions) are sometimes
 * created by this, especially if quick_flag is false.
 *
 * Return true if expression was simplified.
 */
int
div_remainder(equation, np, poly_flag, quick_flag)
token_type	*equation;
int		*np;
int		poly_flag;	/* if true, try polynomial division first, then smart division */
int		quick_flag;	/* if true, keep algebraic fractions simpler */
{
	int	rv = false;

	debug_string(3, "Entering div_remainder().");
	if (quick_flag)
		group_proc(equation, np);
	rv = pdiv_recurse(equation, np, 0, 1, poly_flag);
	if (quick_flag)
		organize(equation, np);
	debug_string(3, "Leaving div_remainder().");
	return rv;
}

static int
pdiv_recurse(equation, np, loc, level, code)
token_type	*equation;
int		*np, loc, level, code;
{
	int	modified = false;
	int	i, j, k;
	int	op, op2, last_op2;
	int	len1, len2, real_len1;
	int	rv = 0;
	int	flag, power_flag, zero_remainder;

	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		if (!(equation[i].level == level && equation[i].token.operatr == DIVIDE))
			continue;
		for (k = i + 2;; k += 2) {
			if (k >= *np || equation[k].level <= level)
				break;
		}
		len1 = real_len1 = k - (i + 1);
		last_op2 = 0;
		for (j = loc; j < *np && equation[j].level >= level; j++) {
			if (equation[j].level == level && equation[j].kind == OPERATOR) {
				last_op2 = equation[j].token.operatr;
				continue;
			}
			if (last_op2 == DIVIDE) {
				continue;
			}
			last_op2 = DIVIDE;
			op = 0;
			for (k = j + 1; k < *np && equation[k].level > level; k += 2) {
				if (equation[k].level == (level + 1)) {
					op = equation[k].token.operatr;
				}
			}
			if (op != PLUS && op != MINUS) {
				continue;
			}
			len2 = k - j;
			flag = code;
			power_flag = false;
			op = 0;
			op2 = 0;
			for (k = i + 2; k < *np && equation[k].level > level; k += 2) {
				if (equation[k].level == level + 3) {
					switch (equation[k].token.operatr) {
					case PLUS:
					case MINUS:
						op2 = PLUS;
					}
				} else if (equation[k].level == level + 2) {
					op = equation[k].token.operatr;
				} else if (equation[k].level == level + 1) {
					if (equation[k].token.operatr == POWER
					    && (op == PLUS || op == MINUS || (op == TIMES && op2 == PLUS))) {
						power_flag = true;
						len1 = k - (i + 1);
					}
					break;
				}
			}
next_thingy:
			if (!power_flag) {
				len1 = real_len1;
			}
			if (flag || power_flag) {
				rv = poly_div(&equation[j], len2, &equation[i+1], len1, NULL);
			} else {
				rv = smart_div(&equation[j], len2, &equation[i+1], len1);
			}
			zero_remainder = (rv > 0 && REMAINDER_IS_ZERO());
			if (power_flag && !zero_remainder) {
				rv = 0;
			}
			if (rv > 0) { /* if successful and the result is smaller than the original: */
				if ((n_tlhs + 2 + n_trhs + len1) > n_tokens)
					error_huge();
				for (k = 0; k < n_tlhs; k++)
					tlhs[k].level++;
				tlhs[n_tlhs].level = 1;
				tlhs[n_tlhs].kind = OPERATOR;
				tlhs[n_tlhs].token.operatr = PLUS;
				n_tlhs++;
				for (k = 0; k < n_trhs; k++)
					trhs[k].level += 2;
				blt(&tlhs[n_tlhs], trhs, n_trhs * sizeof(token_type));
				n_tlhs += n_trhs;
				tlhs[n_tlhs].level = 2;
				tlhs[n_tlhs].kind = OPERATOR;
				tlhs[n_tlhs].token.operatr = DIVIDE;
				n_tlhs++;
				k = n_tlhs;
				blt(&tlhs[n_tlhs], &equation[i+1], len1 * sizeof(token_type));
				n_tlhs += len1;
				for (; k < n_tlhs; k++)
					tlhs[k].level += 2;
				side_debug(3, &equation[j], len2);
				side_debug(3, &equation[i+1], len1);
				simpb_side(tlhs, &n_tlhs, false, true, 3);	/* parameters should match what's used in simpa_side() */
				side_debug(3, tlhs, n_tlhs);
				if (power_flag) {
					k = (var_count(tlhs, n_tlhs) <= var_count(&equation[j], len2));
				} else {
					k = ((var_count(tlhs, n_tlhs) + (n_tlhs >= len1 + 1 + len2)) <= (var_count(&equation[j], len2) + var_count(&equation[i+1], len1)));
				}
				if (k) {
					for (k = 0; k < n_tlhs; k++)
						tlhs[k].level += level;
					if (power_flag) {
						if ((*np - len2 + n_tlhs + 2) > n_tokens)
							error_huge();
						for (k = i + 2 + len1; k <= i + real_len1; k++) {
							equation[k].level++;
						}
						blt(&equation[i+real_len1+3], &equation[k], (*np - k) * sizeof(token_type));
						*np += 2;
						equation[k].level = level + 2;
						equation[k].kind = OPERATOR;
						equation[k].token.operatr = MINUS;
						k++;
						equation[k].level = level + 2;
						equation[k].kind = CONSTANT;
						equation[k].token.constant = 1.0;
						if (i < j) {
							j += 2;
						}
					} else {
						if ((*np - (len1 + 1 + len2) + n_tlhs) > n_tokens)
							error_huge();
						blt(&equation[i], &equation[i+1+len1], (*np - (i + 1 + len1)) * sizeof(token_type));
						*np -= len1 + 1;
						if (i < j) {
							j -= len1 + 1;
						}
					}
					blt(&equation[j+n_tlhs], &equation[j+len2], (*np - (j + len2)) * sizeof(token_type));
					*np -= len2 - n_tlhs;
					blt(&equation[j], tlhs, n_tlhs * sizeof(token_type));
					if (flag || power_flag) {
						debug_string(1, _("Polynomial division successful."));
					} else {
						debug_string(1, _("Smart division successful."));
					}
					side_debug(3, equation, *np);
					return true;
				}
			}
			if (power_flag) {
				power_flag = false;
				goto next_thingy;
			}
			if (flag == code) {
				flag = !flag;
				goto next_thingy;
			}
		}
	}
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= pdiv_recurse(equation, np, i, level + 1, code);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	return modified;
}

/*
 * Do generalized and accurate polynomial division.
 * This is the main polynomial division function for Mathomatic.
 * Works with any number of variables and any pair of expressions.
 *
 * Returns non-zero if successful: 2 if remainder is 0,
 * 1 if result is smaller than the original two expressions,
 * negative if result is larger.
 * Quotient is returned in tlhs[] and remainder in trhs[].
 *
 * If *vp is 0, automatically select the best polynomial base variable and return it in *vp.
 */
int
poly_div(d1, len1, d2, len2, vp)
token_type	*d1;		/* pointer to dividend */
int		len1;		/* length of dividend */
token_type	*d2;		/* pointer to divisor */
int		len2;		/* length of divisor */
long		*vp;		/* pointer to polynomial base variable */
{
	int		i;
	int		rv;
	int		old_partial;
	jmp_buf		save_save;

	old_partial = partial_flag;
	partial_flag = false;	/* We want full unfactoring during polynomial division. */
	blt(save_save, jmp_save, sizeof(jmp_save));
	if ((i = setjmp(jmp_save)) != 0) {	/* Trap errors so we almost always return normally. */
		blt(jmp_save, save_save, sizeof(jmp_save));
		partial_flag = old_partial;
		if (i == 13) {	/* critical error code */
			longjmp(jmp_save, i);
		}
		return false;
	}
	rv = poly_div_sub(d1, len1, d2, len2, vp);
	blt(jmp_save, save_save, sizeof(jmp_save));
	partial_flag = old_partial;
	return rv;
}

#define	MAX_GREATEST_POWER_TERMS	50	/* a limit to keep polynomial division from taking too long */

/*
 * Do the actual polynomial division using a simple,
 * generalized, polynomial long division algorithm.
 */
static int
poly_div_sub(d1, len1, d2, len2, vp)
token_type	*d1;
int		len1;
token_type	*d2;
int		len2;
long		*vp;
{
	int		i;
	int		t1, len_t1;
	int		t2, len_t2;
	int		sign;
	int		divide_flag;
	double		last_power, divisor_power, d;
	int		sum_size;
	int		last_count, count;	/* dividend number of terms raised to the highest power */
	int		divisor_count;		/* divisor number of terms raised to the highest power */
	long		tmp_v = 0;

	if (vp == NULL)
		vp = &tmp_v;
	if (len1 > n_tokens || len2 > n_tokens)
		return false;
	/* Copy the source polynomials to where we want them (tlhs and trhs). */
	if (trhs != d1) {
		blt(trhs, d1, len1 * sizeof(token_type));
	}
	n_trhs = len1;
	if (tlhs != d2) {
		blt(tlhs, d2, len2 * sizeof(token_type));
	}
	n_tlhs = len2;
	/* Do the basic unfactoring and simplification of the dividend and divisor. */
	uf_simp(trhs, &n_trhs);
	uf_simp(tlhs, &n_tlhs);
	if (*vp == 0) {
		/* Select the best polynomial base variable. */
		if (!find_highest_count(trhs, n_trhs, tlhs, n_tlhs, vp))
			return false;
	}
#if	!SILENT
	/* Display debugging info. */
	if (debug_level >= 3) {
		list_var(*vp, 0);
		fprintf(gfp, _("poly_div() starts using base variable %s:\n"), var_str);
		side_debug(3, trhs, n_trhs);
		side_debug(3, tlhs, n_tlhs);
	}
#endif
	/* Determine divide_flag and if the polynomials can be divided. */
	divide_flag = 2;
	last_count = find_greatest_power(trhs, n_trhs, vp, &last_power, &t1, &len_t1, &divide_flag);
	divisor_count = find_greatest_power(tlhs, n_tlhs, vp, &divisor_power, &t2, &len_t2, &divide_flag);
	if (divisor_power <= 0 || last_power < divisor_power) {
		divide_flag = !divide_flag;
		last_count = find_greatest_power(trhs, n_trhs, vp, &last_power, &t1, &len_t1, &divide_flag);
		divisor_count = find_greatest_power(tlhs, n_tlhs, vp, &divisor_power, &t2, &len_t2, &divide_flag);
		if (divisor_power <= 0 || last_power < divisor_power) {
			return false;
		}
	}
	if (divisor_count > 1 || last_count > MAX_GREATEST_POWER_TERMS)	/* Need recursion and factoring to do more than this. */
		return false;

	/* Initialize the quotient. */
	n_quotient = 1;
	quotient[0] = zero_token;
	/* Store the divisor. */
	if (n_tlhs > ARR_CNT(divisor))
		return false;
	blt(divisor, tlhs, n_tlhs * sizeof(token_type));
	n_divisor = n_tlhs;
	sum_size = n_trhs + n_quotient;
	/* Loop until polynomial division is finished. */
	for (;;) {
		if (t1 > 0 && trhs[t1-1].token.operatr == MINUS)
			sign = MINUS;
		else
			sign = PLUS;
		if (t2 > 0 && divisor[t2-1].token.operatr == MINUS) {
			if (sign == MINUS)
				sign = PLUS;
			else
				sign = MINUS;
		}
		if ((len_t1 + len_t2 + 1) > n_tokens)
			return false;
		blt(tlhs, &trhs[t1], len_t1 * sizeof(token_type));
		n_tlhs = len_t1;
		for (i = 0; i < n_tlhs; i++)
			tlhs[i].level++;
		tlhs[n_tlhs].level = 1;
		tlhs[n_tlhs].kind = OPERATOR;
		tlhs[n_tlhs].token.operatr = DIVIDE;
		n_tlhs++;
		blt(&tlhs[n_tlhs], &divisor[t2], len_t2 * sizeof(token_type));
		i = n_tlhs;
		n_tlhs += len_t2;
		for (; i < n_tlhs; i++)
			tlhs[i].level++;
		if (!simp_loop(tlhs, &n_tlhs))
			return false;
		if ((n_quotient + 1 + n_tlhs) > min(ARR_CNT(quotient), n_tokens))
			return false;
		for (i = 0; i < n_tlhs; i++)
			tlhs[i].level++;
		quotient[n_quotient].level = 1;
		quotient[n_quotient].kind = OPERATOR;
		quotient[n_quotient].token.operatr = sign;
		n_quotient++;
		blt(&quotient[n_quotient], tlhs, n_tlhs * sizeof(token_type));
		n_quotient += n_tlhs;
		if ((n_trhs + n_tlhs + n_divisor + 2) > n_tokens)
			return false;
		blt(&trhs[t1+1], &trhs[t1+len_t1], (n_trhs - (t1 + len_t1)) * sizeof(token_type));
		n_trhs -= len_t1 - 1;
		trhs[t1] = zero_token;
		for (i = 0; i < n_trhs; i++)
			trhs[i].level++;
		trhs[n_trhs].level = 1;
		trhs[n_trhs].kind = OPERATOR;
		if (sign == PLUS)
			trhs[n_trhs].token.operatr = MINUS;
		else
			trhs[n_trhs].token.operatr = PLUS;
		n_trhs++;
		blt(&trhs[n_trhs], tlhs, n_tlhs * sizeof(token_type));
		i = n_trhs;
		n_trhs += n_tlhs;
		for (; i < n_trhs; i++)
			trhs[i].level++;
		trhs[n_trhs].level = 2;
		trhs[n_trhs].kind = OPERATOR;
		trhs[n_trhs].token.operatr = TIMES;
		n_trhs++;
		i = n_trhs;
		blt(&trhs[n_trhs], divisor, t2 * sizeof(token_type));
		n_trhs += t2;
		trhs[n_trhs] = zero_token;
		n_trhs++;
		blt(&trhs[n_trhs], &divisor[t2+len_t2], (n_divisor - (t2 + len_t2)) * sizeof(token_type));
		n_trhs += (n_divisor - (t2 + len_t2));
		for (; i < n_trhs; i++)
			trhs[i].level += 2;
		side_debug(3, trhs, n_trhs);
		uf_repeat(trhs, &n_trhs);
		uf_tsimp(trhs, &n_trhs);
		side_debug(4, trhs, n_trhs);
		count = find_greatest_power(trhs, n_trhs, vp, &d, &t1, &len_t1, &divide_flag);
		if (d < divisor_power) {
			/* Success!  Polynomial division ends here. */
			debug_string(3, "Successful polynomial division!");
			blt(tlhs, quotient, n_quotient * sizeof(token_type));
			n_tlhs = n_quotient;
			debug_string(3, "Quotient:");
			side_debug(3, tlhs, n_tlhs);
			debug_string(3, "Remainder:");
			side_debug(3, trhs, n_trhs);
			if (REMAINDER_IS_ZERO())
				return 2;
			if ((n_trhs + n_quotient) >= (sum_size /* - (sum_size / 10) */)) {
				if ((n_trhs + 1) > sum_size && n_trhs > n_divisor)
					return -2;
				else
					return -1;
			}
			return 1;
		} else if (d < last_power) {
			last_power = d;
			last_count = count;
			if (last_count > MAX_GREATEST_POWER_TERMS)	/* Need factoring to do more than this. */
				return false;
		} else if (d > last_power) {
			return false;
		} else {
			if (count >= last_count) {
				return false;
			}
			last_count = count;
		}
	}
}

/*
 * Do smart division.
 *
 * Smart division is heuristic division much like polynomial division,
 * however instead of basing the division on the highest powers of a base variable,
 * every term in the dividend is tried, and if a trial makes the
 * expression smaller, we go with that.
 *
 * Only one term in the divisor is tried, to save time.
 * Currently, the divisor term used is the one with the least number
 * of variables, not including any terms with no variables.
 *
 * Returns true if successful.
 * Quotient is returned in tlhs[] and remainder in trhs[].
 */
int
smart_div(d1, len1, d2, len2)
token_type	*d1;		/* pointer to dividend */
int		len1;		/* length of dividend */
token_type	*d2;		/* pointer to divisor */
int		len2;		/* length of divisor */
{
	int		i, j, k;
	int		t1, len_t1;
	int		t2 = 0, len_t2 = 0;
	int		sign;
	int		old_n_quotient;
	int		trhs_size;
	int		term_size = 0, term_count;
	int		term_pos = 0, skip_terms[100];
	int		skip_count;
	token_type	*qp;
	int		q_size;
	int		sum_size;
	int		count;
	int		dcount = 0;		/* divisor term count */
	int		flag;

	blt(trhs, d1, len1 * sizeof(token_type));
	n_trhs = len1;
	blt(tlhs, d2, len2 * sizeof(token_type));
	n_tlhs = len2;
	/* Do the basic unfactoring and simplification of the dividend and divisor. */
	uf_simp_no_repeat(trhs, &n_trhs);
	uf_simp_no_repeat(tlhs, &n_tlhs);
	/* Display debugging info. */
	debug_string(3, "smart_div() starts:");
	side_debug(3, trhs, n_trhs);
	side_debug(3, tlhs, n_tlhs);
	/* Find which divisor term to use. */
	for (i = 0, j = 0, k = 0, flag = false;; i++) {
		if (i >= n_tlhs || (tlhs[i].kind == OPERATOR && tlhs[i].level == 1
		    && (tlhs[i].token.operatr == PLUS || tlhs[i].token.operatr == MINUS))) {
			dcount++;
			if (flag) {
				if (len_t2 == 0 || var_count(&tlhs[j], i - j) < k) {
					len_t2 = i - j;
					t2 = j;
					k = var_count(&tlhs[t2], len_t2);
				}
			}
			flag = false;
			j = i + 1;
		} else if (tlhs[i].kind == VARIABLE && tlhs[i].token.variable != IMAGINARY) {
			flag = true;
		}
		if (i >= n_tlhs)
			break;
	}
	if (len_t2 <= 0)
		return false;
	/* Initialize the quotient. */
	n_quotient = 1;
	quotient[0] = zero_token;
	if (n_tlhs > ARR_CNT(divisor))
		return false;
	blt(divisor, tlhs, n_tlhs * sizeof(token_type));
	n_divisor = n_tlhs;
try_one:
	trhs_size = n_trhs;
	for (skip_count = 0, count = 0;;) {
		sum_size = n_trhs + n_quotient;
		for (term_count = 1, q_size = 0;; term_count++) {
			if (!get_term(trhs, n_trhs, term_count, &t1, &len_t1))
				break;
			flag = false;
			for (i = 0; i < skip_count; i++) {
				if (skip_terms[i] == t1) {
					flag = true;
					break;
				}
			}
			if (flag)
				continue;
			if ((len_t1 + len_t2 + 1) > n_tokens)
				return false;
			blt(tlhs, &trhs[t1], len_t1 * sizeof(token_type));
			n_tlhs = len_t1;
			for (i = 0; i < n_tlhs; i++)
				tlhs[i].level++;
			tlhs[n_tlhs].level = 1;
			tlhs[n_tlhs].kind = OPERATOR;
			tlhs[n_tlhs].token.operatr = DIVIDE;
			n_tlhs++;
			blt(&tlhs[n_tlhs], &divisor[t2], len_t2 * sizeof(token_type));
			i = n_tlhs;
			n_tlhs += len_t2;
			for (; i < n_tlhs; i++)
				tlhs[i].level++;
			if (!simp_loop(tlhs, &n_tlhs))
				continue;
			if (basic_size(tlhs, n_tlhs) <= basic_size(&trhs[t1], len_t1)) {
				q_size = n_tlhs;
				term_pos = t1;
				term_size = len_t1;
				break;
			}
		}
		if (q_size <= 0) {
			if (count <= 0) {
				if (dcount > 1) {
					dcount = 1;
					t2 = 0;
					len_t2 = n_divisor;
					goto try_one;	/* Try using the whole divisor as the divisor term. */
				}
				return false;
			}
end_div:
			if (dcount > 1) {
				if (n_quotient + n_trhs >= trhs_size + 1) {
					return false;
				}
			}
end_div2:
			blt(tlhs, quotient, n_quotient * sizeof(token_type));
			n_tlhs = n_quotient;
			side_debug(3, tlhs, n_tlhs);
			side_debug(3, trhs, n_trhs);
			return true;	/* Success! */
		}
		t1 = term_pos;
		len_t1 = term_size;
		if (t1 > 0 && trhs[t1-1].token.operatr == MINUS)
			sign = MINUS;
		else
			sign = PLUS;
		if (t2 > 0 && divisor[t2-1].token.operatr == MINUS) {
			if (sign == MINUS)
				sign = PLUS;
			else
				sign = MINUS;
		}
		if ((len_t1 + len_t2 + 1) > n_tokens)
			return false;
		blt(tlhs, &trhs[t1], len_t1 * sizeof(token_type));
		n_tlhs = len_t1;
		for (i = 0; i < n_tlhs; i++)
			tlhs[i].level++;
		tlhs[n_tlhs].level = 1;
		tlhs[n_tlhs].kind = OPERATOR;
		tlhs[n_tlhs].token.operatr = DIVIDE;
		n_tlhs++;
		blt(&tlhs[n_tlhs], &divisor[t2], len_t2 * sizeof(token_type));
		i = n_tlhs;
		n_tlhs += len_t2;
		for (; i < n_tlhs; i++)
			tlhs[i].level++;
		simp_loop(tlhs, &n_tlhs);
		if ((n_quotient + 1 + n_tlhs) > min(ARR_CNT(quotient), n_tokens))
			return false;
		for (i = 0; i < n_tlhs; i++)
			tlhs[i].level++;
		old_n_quotient = n_quotient;
		quotient[n_quotient].level = 1;
		quotient[n_quotient].kind = OPERATOR;
		quotient[n_quotient].token.operatr = sign;
		n_quotient++;
		qp = &quotient[n_quotient];
		q_size = n_tlhs;
		blt(&quotient[n_quotient], tlhs, n_tlhs * sizeof(token_type));
		n_quotient += n_tlhs;
		if ((n_trhs + q_size + n_divisor + 2) > n_tokens)
			return false;
		blt(tlhs, trhs, n_trhs * sizeof(token_type));
		n_tlhs = n_trhs;
		blt(&trhs[t1+1], &trhs[t1+len_t1], (n_trhs - (t1 + len_t1)) * sizeof(token_type));
		n_trhs -= len_t1 - 1;
		trhs[t1] = zero_token;
		for (i = 0; i < n_trhs; i++)
			trhs[i].level++;
		trhs[n_trhs].level = 1;
		trhs[n_trhs].kind = OPERATOR;
		if (sign == PLUS)
			trhs[n_trhs].token.operatr = MINUS;
		else
			trhs[n_trhs].token.operatr = PLUS;
		n_trhs++;
		blt(&trhs[n_trhs], qp, q_size * sizeof(token_type));
		i = n_trhs;
		n_trhs += q_size;
		for (; i < n_trhs; i++)
			trhs[i].level++;
		trhs[n_trhs].level = 2;
		trhs[n_trhs].kind = OPERATOR;
		trhs[n_trhs].token.operatr = TIMES;
		n_trhs++;
		i = n_trhs;
		blt(&trhs[n_trhs], divisor, t2 * sizeof(token_type));
		n_trhs += t2;
		trhs[n_trhs] = zero_token;
		n_trhs++;
		blt(&trhs[n_trhs], &divisor[t2+len_t2], (n_divisor - (t2 + len_t2)) * sizeof(token_type));
		n_trhs += (n_divisor - (t2 + len_t2));
		for (; i < n_trhs; i++)
			trhs[i].level += 2;
		side_debug(3, trhs, n_trhs);
		uf_tsimp(trhs, &n_trhs);
		side_debug(4, trhs, n_trhs);
		if (REMAINDER_IS_ZERO())
			goto end_div2;
		if (dcount > 1) {
			if ((n_trhs + n_quotient) >= sum_size) {
				if (skip_count >= ARR_CNT(skip_terms)) {
					if (count == 0) {
						return false;
					} else {
						n_quotient = old_n_quotient;
						blt(trhs, tlhs, n_tlhs * sizeof(token_type));
						n_trhs = n_tlhs;
						goto end_div;
					}
				}
				skip_terms[skip_count] = term_pos;
				skip_count++;
				n_quotient = old_n_quotient;
				blt(trhs, tlhs, n_tlhs * sizeof(token_type));
				n_trhs = n_tlhs;
				debug_string(3, "Skipping last operation.");
				continue;
			}
		}
		if (n_trhs == 1 && trhs[0].kind == CONSTANT)
			goto end_div;
		skip_count = 0;
		count++;
	}
}

/*
 * Return the size of a sub-expression,
 * minus any constant multiplier.
 */
int
basic_size(p1, len)
token_type	*p1;
int		len;
{
	int	i, j;
	int	level;
	int	rv;
	int	constant_flag = true;

	rv = len;
	level = min_level(p1, len);
	for (i = 0, j = -1; i < len; i++) {
		if (p1[i].kind == OPERATOR) {
			if (p1[i].level == level
			    && (p1[i].token.operatr == TIMES || p1[i].token.operatr == DIVIDE)) {
				if (constant_flag) {
					rv -= i - j;
				}
				j = i;
				constant_flag = true;
			}
		} else if (p1[i].kind != CONSTANT)
			constant_flag = false;
	}
	if (constant_flag) {
		rv -= i - j;
	}
	return rv;
}

int
get_term(p1, n1, count, tp1, lentp1)
token_type	*p1;
int		n1;
int		count;
int		*tp1, *lentp1;
{
	int	i, j;
	int	no;

	for (no = 0, i = 1, j = 0;; i += 2) {
		if (i >= n1 || (p1[i].level == 1
		    && (p1[i].token.operatr == PLUS
		    || p1[i].token.operatr == MINUS))) {
			no++;
			if (no >= count) {
				*tp1 = j;
				*lentp1 = i - j;
				return true;
			}
			j = i + 1;
		}
		if (i >= n1)
			return false;
	}
}

/*
 * This routine automatically finds the best variable to do polynomial division with.
 *
 * Returns 0 if nothing was found,
 * otherwise the selected polynomial base variable is returned in *vp1,
 * and the number of times it occurs in the dividend is returned.
 */
static int
find_highest_count(p1, n1, p2, n2, vp1)
token_type	*p1;		/* pointer to dividend expression */
int		n1;		/* length of dividend */
token_type	*p2;		/* pointer to divisor expression */
int		n2;		/* length of divisor */
long		*vp1;		/* variable pointer to return base variable with */
{
	int		i;
	int		vc, cnt;
	long		v1, last_v, cv;		/* Mathomatic variables */
	sort_type	va[MAX_VARS];
	int		divide_flag;
	int		t1, len_t1;
	int		t2, len_t2;
	double		d1, d2;
	int		count1, count2;

	last_v = 0;
	for (vc = 0; vc < ARR_CNT(va);) {
		cnt = 0;
		v1 = -1;
		for (i = 0; i < n1; i += 2) {
			if (p1[i].kind == VARIABLE && p1[i].token.variable > last_v) {
				if (v1 == -1 || p1[i].token.variable < v1) {
					v1 = p1[i].token.variable;
					cnt = 1;
				} else if (p1[i].token.variable == v1) {
					cnt++;
				}
			}
		}
		if (v1 == -1)
			break;
		last_v = v1;
		va[vc].v = v1;
		va[vc].count = cnt;
		vc++;
	}
	if (vc <= 0)
		return 0;
	qsort((char *) va, vc, sizeof(*va), vcmp);
	for (cv = IMAGINARY + 1; cv > 0; cv--) {
		for (i = 0; i < vc; i++) {
			if ((cv > IMAGINARY) ? ((va[i].v & VAR_MASK) <= SIGN) : (va[i].v != cv)) {
				continue;
			}
			*vp1 = va[i].v;
			divide_flag = 2;
			count1 = find_greatest_power(p1, n1, vp1, &d1, &t1, &len_t1, &divide_flag);
			count2 = find_greatest_power(p2, n2, vp1, &d2, &t2, &len_t2, &divide_flag);
			if (d2 <= 0 || d1 < d2 || count2 > count1) {
				divide_flag = !divide_flag;
				count1 = find_greatest_power(p1, n1, vp1, &d1, &t1, &len_t1, &divide_flag);
				count2 = find_greatest_power(p2, n2, vp1, &d2, &t2, &len_t2, &divide_flag);
				if (d2 <= 0 || d1 < d2 || count2 > count1) {
					continue;
				}
			}
			return va[i].count;
		}
#if	1
		break;
#endif
	}
	return 0;
}

#define	VALUE_CNT	3

void
term_value(dp, p1, n1, loc)
double		*dp;
token_type	*p1;
int		n1, loc;
{
	int	i, j, k;
	int	divide_flag = false;
	int	level, div_level = 0;
	double	d, sub_count, sub_sum;

	for (i = 0; i < VALUE_CNT; i++)
		dp[i] = 0.0;
	for (i = loc; i < n1; i++) {
		level = p1[i].level;
		if (p1[i].kind == VARIABLE) {
			if (divide_flag) {
				dp[0] -= 1.0;
				dp[1] -= p1[i].token.variable;
				dp[2] -= p1[i].token.variable;
			} else {
				dp[0] += 1.0;
				dp[1] += p1[i].token.variable;
				dp[2] += p1[i].token.variable;
			}
		} else if (p1[i].kind == OPERATOR) {
			if (level == 1 && (p1[i].token.operatr == PLUS || p1[i].token.operatr == MINUS))
				break;
			if (p1[i].token.operatr == DIVIDE) {
				if (divide_flag && level >= div_level)
					continue;
				div_level = level;
				divide_flag = true;
			} else if (divide_flag && level <= div_level) {
				divide_flag = false;
			}
		}
	}
	divide_flag = false;
	for (j = loc + 1; j < i; j += 2) {
		level = p1[j].level;
		if (p1[j].token.operatr == DIVIDE) {
			if (divide_flag && level >= div_level)
				continue;
			div_level = level;
			divide_flag = true;
		} else if (divide_flag && level <= div_level) {
			divide_flag = false;
		}
		if (p1[j].token.operatr == POWER && level == p1[j+1].level && p1[j+1].kind == CONSTANT) {
			d = p1[j+1].token.constant - 1.0;
			sub_count = 0.0;
			sub_sum = 0.0;
			for (k = j - 1; k >= loc && p1[k].level >= level; k--) {
				if (p1[k].kind == VARIABLE) {
					sub_count += 1;
					sub_sum += p1[k].token.variable;
				}
			}
			if (divide_flag) {
				dp[0] -= d * sub_count;
				dp[2] -= d * sub_sum;
			} else {
				dp[0] += d * sub_count;
				dp[2] += d * sub_sum;
			}
		}
	}
}

/*
 * This routine finds the additive term raised the highest power,
 * with ordering and much flexibility.
 *
 * The number of terms raised to the highest power is returned, unless *vp1 == 0,
 * then 0 is returned and *vp1 is set to a valid polynomial base variable, if any.
 */
int
find_greatest_power(p1, n1, vp1, pp1, tp1, lentp1, dcodep)
token_type	*p1;		/* pointer to expression */
int		n1;		/* length of expression */
long		*vp1;		/* polynomial base variable */
double		*pp1;		/* returned power of the returned term, which will be the highest power */
int		*tp1, *lentp1;	/* the returned term index and length */
int		*dcodep;	/* divide flag pointer indicates if term is a denominator; */
				/* for example: for x^5 it is false, for 1/x^5 it is true. */
				/* If equal to 2, set it; if equal to 3, ignore it. */
{
	int		i, j, k, ii;
	double		d;
	int		flag, divide_flag = false;
	int		level, div_level = 0;
	long		v = 0;
	int		was_power = false;
	double		last_va[VALUE_CNT];
	double		va[VALUE_CNT];
	int		rv;
	int		count = 0;

	CLEAR_ARRAY(last_va);
	*pp1 = 0.0;
	*tp1 = -1;
	*lentp1 = 0;
	rv = *dcodep;
	for (j = 0, i = 1;; i += 2) {
		if (i >= n1 || ((p1[i].token.operatr == PLUS || p1[i].token.operatr == MINUS) && p1[i].level == 1)) {
			divide_flag = false;
			if (!was_power && *pp1 <= 1.0) {
				for (k = j; k < i; k++) {
					if (p1[k].kind == VARIABLE) {
						if (*dcodep <= 1 && *dcodep != divide_flag)
							continue;
						if (*vp1) {
							if (p1[k].token.variable == *vp1) {
								term_value(va, p1, n1, j);
								flag = (*pp1 == 1.0 && (rv > divide_flag));
								if (*pp1 == 1.0 && rv == divide_flag) {
									if (*tp1 != j)
										count++;
									for (ii = 0; ii < ARR_CNT(va); ii++) {
										if (va[ii] == last_va[ii])
											continue;
										if (va[ii] < last_va[ii])
											flag = true;
										break;
									}
								} else if (*pp1 < 1.0 || flag) {
									count = 1;
								}
								if (*pp1 < 1.0 || flag) {
									blt(last_va, va, sizeof(last_va));
									*pp1 = 1.0;
									*tp1 = j;
									rv = divide_flag;
								}
								break;
							}
						} else if ((p1[k].token.variable & VAR_MASK) > SIGN) {
							v = p1[k].token.variable;
							*pp1 = 1.0;
							*tp1 = j;
							rv = divide_flag;
							break;
						}
					} else if (p1[k].kind == OPERATOR) {
						if (p1[k].token.operatr == DIVIDE) {
							if (divide_flag && p1[k].level >= div_level)
								continue;
							div_level = p1[k].level;
							divide_flag = true;
						} else if (divide_flag && p1[k].level <= div_level) {
							divide_flag = false;
						}
						if (p1[k].token.operatr == POWER) {
							level = p1[k].level;
							do {
								k += 2;
							} while (k < i && p1[k].level > level);
							k--;
						}
					}
				}
			}
			if (i >= n1)
				break;
			j = i + 1;
			was_power = false;
			divide_flag = false;
			continue;
		}
		level = p1[i].level;
		if (p1[i].token.operatr == DIVIDE) {
			if (divide_flag && level >= div_level)
				continue;
			div_level = level;
			divide_flag = true;
		} else if (divide_flag && level <= div_level) {
			divide_flag = false;
		}
		if (p1[i].token.operatr == POWER && p1[i+1].kind == CONSTANT
		    && (*vp1 || level == p1[i+1].level)) {
			if (*dcodep <= 1 && *dcodep != divide_flag)
				continue;
			d = p1[i+1].token.constant;
			for (k = i;;) {
				if (p1[k-1].kind == VARIABLE) {
					if (*vp1) {
						if (p1[k-1].token.variable == *vp1) {
							was_power = true;
							term_value(va, p1, n1, j);
							flag = (d == *pp1 && (rv > divide_flag));
							if (d == *pp1 && rv == divide_flag) {
								if (*tp1 != j)
									count++;
								for (ii = 0; ii < ARR_CNT(va); ii++) {
									if (va[ii] == last_va[ii])
										continue;
									if (va[ii] < last_va[ii])
										flag = true;
									break;
								}
							} else if (d > *pp1 || flag) {
								count = 1;
							}
							if (d > *pp1 || flag) {
								blt(last_va, va, sizeof(last_va));
								*pp1 = d;
								*tp1 = j;
								rv = divide_flag;
							}
							break;
						}
					} else if ((p1[k-1].token.variable & VAR_MASK) > SIGN) {
						was_power = true;
						if (d > *pp1) {
							v = p1[k-1].token.variable;
							*pp1 = d;
							*tp1 = j;
							rv = divide_flag;
						}
						break;
					}
				}
				k -= 2;
				if (k <= j)
					break;
				if (p1[k].level <= level)
					break;
			}
		}
	}
	if (*vp1 == 0) {
		*vp1 = v;
	}
	if (*tp1 >= 0) {
		for (i = *tp1 + 1; i < n1; i += 2) {
			if ((p1[i].token.operatr == PLUS || p1[i].token.operatr == MINUS)
			    && p1[i].level == 1) {
				break;
			}
		}
		*lentp1 = i - *tp1;
	}
	if (*dcodep == 2)
		*dcodep = rv;
	return count;
}
