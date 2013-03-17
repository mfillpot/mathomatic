/*
 * Mathomatic simplifying routines.
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

/*
 * MAX_COMPARE_TERMS defines the maximum number of terms
 * on the same level of parentheses that can be compared.
 * Most of the stack usage is related this value.
 * Space for this many pointers is reserved on the stack
 * with each level of parentheses while comparing two expressions.
 */
#define	MAX_COMPARE_TERMS	(DEFAULT_N_TOKENS / 6)

static int org_recurse(token_type *equation, int *np, int loc, int level, int *elocp);
static int const_recurse(token_type *equation, int *np, int loc, int level, int iflag);
static int compare_recurse(token_type *p1, int n1, int l1, token_type *p2, int n2, int l2, int *diff_signp);
static int order_recurse(token_type *equation, int *np, int loc, int level);

/*
 * Fix up levels of parentheses in an equation side.
 * Remove unnecessary parentheses and put similar operators on the same level.
 *
 * This is the inner-most loop in Mathomatic, make it fast.
 */
void
organize(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
#if	DEBUG
	if (equation == NULL || np == NULL) {
		error_bug("NULL pointer passed to organize().");
	}
#endif
	if (*np <= 0 || (*np & 1) == 0) {
		printf("Bad expression size = %d.\n", *np);
		error_bug("Internal error: organize() called with bad expression size.");
	}
	if (*np > n_tokens) {
		error_bug("Internal error: expression array overflow detected in organize().");
	}
	org_recurse(equation, np, 0, 1, NULL);
}

static inline void
org_up_level(bp, ep, level, invert)
token_type	*bp, *ep;
int		level, invert;
{
	if (invert) {
		for (; bp <= ep; bp++) {
			bp->level--;
			if (bp->level == level && bp->kind == OPERATOR) {
				switch (bp->token.operatr) {
				case PLUS:
					bp->token.operatr = MINUS;
					break;
				case MINUS:
					bp->token.operatr = PLUS;
					break;
				case TIMES:
					bp->token.operatr = DIVIDE;
					break;
				case DIVIDE:
					bp->token.operatr = TIMES;
					break;
				}
			}
		}
	} else {
		for (; bp <= ep; bp++) {
			bp->level--;
		}
	}
}

/*
 * Recurse through every sub-expression in "equation", starting at "loc",
 * moving up levels to "level" of parentheses.
 */
static int
org_recurse(equation, np, loc, level, elocp)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
int		loc, level, *elocp;
{
	token_type	*p1, *bp, *ep;
	int		op, sub_op;
	int		i;
	int		eloc;
	int		sub_eloc;
	int		min1;
	int		invert;

	bp = &equation[loc];
	ep = &equation[*np];
	min1 = bp->level;
	for (p1 = bp + 1; p1 < ep; p1 += 2) {
		if (p1->level < min1) {
			if (p1->level < level)
				break;
			min1 = p1->level;
		}
	}
	ep = p1;
	eloc = (ep - equation) - 1;
	if (elocp)
		*elocp = eloc;
	if (eloc == loc) {
		bp->level = max(level - 1, 1);
		return 0;
	}
	if (min1 > level) {
		for (p1 = bp; p1 < ep; p1++)
			p1->level -= min1 - level;
	}
	op = 0;
	for (p1 = bp + 1; p1 < ep; p1 += 2) {
		if (p1->level == level) {
			op = p1->token.operatr;
			break;
		}
	}
	for (i = loc; i <= eloc; i += 2) {
		if (equation[i].level > level) {
			sub_op = org_recurse(equation, np, i, level + 1, &sub_eloc);
			switch (sub_op) {
			case PLUS:
			case MINUS:
				if (op != PLUS && op != MINUS)
					break;
				invert = (i - 1 >= loc && equation[i-1].token.operatr == MINUS);
				org_up_level(&equation[i], &equation[sub_eloc], level, invert);
				break;
			case TIMES:
			case DIVIDE:
				if (op != TIMES && op != DIVIDE)
					break;
				invert = (i - 1 >= loc && equation[i-1].token.operatr == DIVIDE);
				org_up_level(&equation[i], &equation[sub_eloc], level, invert);
				break;
			}
			i = sub_eloc;
		}
	}
	return op;
}

/*
 * The quickest, most basic simplification loop.
 * Just does constant simplification.
 */
void
elim_loop(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of equation side */
{
	if (abort_flag) {
		/* Control-C pressed, gracefully return to main prompt and leave unsimplified */
		abort_flag = false;
#if	DEBUG && !SILENT
		char	*cp, buf[100];

		my_strlcpy(prompt_str, _("Enter debug level, or an empty line to abort the current operation: "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL || *cp == '\0') {
			longjmp(jmp_save, 13);
		} else {
			debug_level = decstrtol(cp, NULL);
			printf(_("Debug level set to %d.\n"), debug_level);
		}
#else
		longjmp(jmp_save, 13);
#endif
	}
	side_debug(6, equation, *np);
	do {
		do {
			do {
				organize(equation, np);
			} while (combine_constants(equation, np, true));
		} while (elim_k(equation, np));
	} while (simp_pp(equation, np));
	if (reorder(equation, np)) {
		do {
			organize(equation, np);
		} while (elim_k(equation, np));
	}
	side_debug(5, equation, *np);
}

/*
 * Configurable high level simplify routine.
 */
void
simp_ssub(equation, np, v, d, power_flag, times_flag, fc_level)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of equation side */
long		v;		/* variable to factor, 0L or MATCH_ANY to factor all variables */
double		d;		/* factor expressions raised to the power of this if v */
int		power_flag;	/* factor_power() flag */
int		times_flag;	/* factor_times() flag */
int		fc_level;	/* factor constants code, passed to factor_constants() */
{
	do {
		do {
			do {
				do {
					do {
						do {
							do {
								do {
									elim_loop(equation, np);
								} while (simp2_power(equation, np));
							} while (times_flag && factor_times(equation, np));
						} while (elim_sign(equation, np));
					} while (subtract_itself(equation, np));
				} while (factor_constants(equation, np, fc_level));
			} while (factor_divide(equation, np, v, d));
		} while (factor_plus(equation, np, v, d));
	} while (power_flag && factor_power(equation, np));
}

/*
 * Quickly and very basically simplify an equation space.
 * No factoring is done.
 */
void
simp_equation(n)
int	n;	/* equation space number to simplify */
{
	if (empty_equation_space(n))
		return;
	simp_loop(lhs[n], &n_lhs[n]);
	if (n_rhs[n] > 0) {
		simp_loop(rhs[n], &n_rhs[n]);
	}
}

/*
 * For quick, mid-range simplification of an equation side.
 * Trivial factoring is done.
 */
void
mid_simp_side(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of equation side */
{
	simp_ssub(equation, np, 0L, 1.0, true, true, 6);
}

/*
 * For quick, mid-range simplification of an equation space.
 * Trivial factoring is done.
 */
void
mid_simp_equation(n)
int	n;	/* equation space number to simplify */
{
	if (empty_equation_space(n))
		return;
	mid_simp_side(lhs[n], &n_lhs[n]);
	if (n_rhs[n] > 0) {
		mid_simp_side(rhs[n], &n_rhs[n]);
	}
}

/*
 * This function is the mid-range simplifier used by the solver.
 */
void
simps_side(equation, np, zsolve)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of equation side */
int		zsolve;		/* true for solving for zero */
{
	elim_loop(equation, np);
	simp_constant_power(equation, np);
	do {
		simp_ssub(equation, np, 0L, 0.0, !zsolve, true, 6);
	} while (super_factor(equation, np, 0));
}

/*
 * This function is used by the factor command.
 */
void
simpv_side(equation, np, v)
token_type	*equation;	/* pointer to the beginning of equation side to factor */
int		*np;		/* pointer to length of equation side */
long		v;		/* variable to factor, 0 for all variables */
{
	simp_ssub(equation, np, v, 0.0, v == 0, true, 6);
}

/*
 * For factoring an equation space like the factor command.
 * Trivial factoring is done.
 */
void
simpv_equation(n, v)
int	n;	/* equation space number to simplify */
long	v;	/* Mathomatic variable to factor or 0 */
{
	if (empty_equation_space(n))
		return;
	simpv_side(lhs[n], &n_lhs[n], v);
	if (n_rhs[n] > 0) {
		simpv_side(rhs[n], &n_rhs[n], v);
	}
}

/*
 * Factor out and simplify imaginary constants in an equation side.
 * Do complex number root approximation and other complex number simplification.
 *
 * Return true if anything was approximated.
 */
int
factor_imaginary(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to equation side length */
{
	int	rv;

	rv = approximate_complex_roots(equation, np);
	factorv(equation, np, IMAGINARY);
	return rv;
}

/*
 * Factor out only the specified variable "v" and simplify a little.
 * Does not do any approximation.
 *
 * If v is IMAGINARY, simplify imaginary number division, so that the imaginary unit is
 * not in the divisor.
 */
void
factorv(equation, np, v)
token_type	*equation;	/* pointer to the beginning of equation side to factor */
int		*np;		/* pointer to equation side length */
long		v;		/* Mathomatic variable to factor */
{
	do {
		do {
			simp_loop(equation, np);
		} while (factor_plus(equation, np, v, 0.0));
	} while (v == IMAGINARY && div_imaginary(equation, np));
}

/*
 * Simplify and approximate for the calculate command and for more successful comparisons.
 * Includes complex number simplification.
 */
void
calc_simp(equation, np)
token_type	*equation;
int		*np;
{
	approximate_roots = true;
	subst_constants(equation, np);
	mid_simp_side(equation, np);
	factor_imaginary(equation, np);
	ufactor(equation, np);
	factor_imaginary(equation, np);
	uf_simp(equation, np);
	factor_imaginary(equation, np);
	mid_simp_side(equation, np);
	make_simple_fractions(equation, np);
	uf_tsimp(equation, np);
	approximate_roots = false;
}

/*
 * Approximate an equation side for the approximate command.
 */
void
approximate(equation, np)
token_type	*equation;
int		*np;
{
	if (repeat_flag) {
		/* do more */
		calc_simp(equation, np);
	} else {
		subst_constants(equation, np);
		approximate_roots = true;
		simp_loop(equation, np);
		factor_imaginary(equation, np);
		approximate_roots = false;
	}
}

/*
 * Try to eliminate imaginary units (i#) from an equation side by converting
 * expressions like (i#*(b^.5)) to ((-1*b)^.5).
 *
 * Return true if any imaginary units where substituted.
 */
int
simp_i(equation, np)
token_type	*equation;
int		*np;
{
	int	i;
	int	level;
	int	rv = false;

	simp_loop(equation, np);
	for (i = 0; i < *np; i++) {
		switch (equation[i].kind) {
		case VARIABLE:
			if (equation[i].token.variable == IMAGINARY) {
				if (*np + 2 > n_tokens) {
					error_huge();
				}
				level = equation[i].level + 1;
				blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
				*np += 2;
				equation[i].level = level;
				equation[i].kind = CONSTANT;
				equation[i].token.constant = -1.0;
				i++;
				equation[i].level = level;
				equation[i].kind = OPERATOR;
				equation[i].token.operatr = POWER;
				i++;
				equation[i].level = level;
				equation[i].kind = CONSTANT;
				equation[i].token.constant = 0.5;
				rv = true;
			}
			break;
		case OPERATOR:
#if	0
			if (equation[i].token.operatr != POWER)
				continue;
			level = equation[i].level;
/* Skip imaginary units on the right side of any power operator. */
			for (; i < *np && equation[i].level >= level; i += 2)
				;
			i--;
#endif
			break;
		case CONSTANT:
			break;
		}
	}
	do {
		do {
			do {
				do {
					do {
						organize(equation, np);
					} while (combine_constants(equation, np, false));
				} while (elim_k(equation, np));
			} while (simp_pp(equation, np));
/* The following factor_power() messes up absolute values and complex numbers sometimes: */
		} while (factor_power(equation, np));
	} while (factor_times(equation, np));
	simp_loop(equation, np);
	return rv;
}

/*
 * Combine all like denominators.
 */
void
simp_divide(equation, np)
token_type	*equation;
int		*np;
{
	do {
		do {
			simp_loop(equation, np);
		} while (factor_constants(equation, np, 1));
	} while (factor_divide(equation, np, 0L, 0.0));
}

/*
 * Combine all like denominators containing variable "v".
 * Don't call factor_times().
 *
 * For beauty simplifier simpb_side() below.
 */
void
simp2_divide(equation, np, v, fc_level)
token_type	*equation;
int		*np;
long		v;
int		fc_level;
{
	do {
		do {
			do {
				do {
					do {
						elim_loop(equation, np);
					} while (simp2_power(equation, np));
				} while (elim_sign(equation, np));
			} while (subtract_itself(equation, np));
		} while (factor_constants(equation, np, fc_level));
	} while (factor_divide(equation, np, v, 0.0));
}

/*
 * Compare function for qsort(3) within simpb_side() below.
 */
static int
simpb_vcmp(p1, p2)
sort_type	*p1, *p2;
{
	if (((p1->v & VAR_MASK) == SIGN) == ((p2->v & VAR_MASK) == SIGN)) {
		if (p2->count == p1->count) {
			if (p1->v < p2->v)
				return -1;
			if (p1->v == p2->v)
				return 0;
			return 1;
		}
		return(p2->count - p1->count);
	} else {
		if ((p1->v & VAR_MASK) == SIGN) {
			return -1;
		} else {
			return 1;
		}
	}
}

/*
 * Beauty simplifier for equation sides.
 * This is the neat simplify and variable factor routine, to make a pleasing display.
 * Factors variables in order: "sign" variables first, then by frequency.
 */
void
simpb_side(equation, np, uf_power_flag, power_flag, fc_level)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to length of equation side */
int		uf_power_flag;	/* uf_allpower() flag */
int		power_flag;	/* factor_power() flag */
int		fc_level;	/* factor constants code, passed to factor_constants() */
{
	int		i;
	int		vc, cnt;	/* counts */
	long		v1, last_v;	/* Mathomatic variables */
	sort_type	va[MAX_VARS];	/* array of all real variables found in equation side */

	elim_loop(equation, np);
	if (uf_power_flag) {
		uf_allpower(equation, np);
	}
	last_v = 0;
	for (vc = 0; vc < ARR_CNT(va);) {
		cnt = 0;
		v1 = -1;
		for (i = 0; i < *np; i += 2) {
			if (equation[i].kind == VARIABLE && equation[i].token.variable > last_v) {
				if (v1 == -1 || equation[i].token.variable < v1) {
					v1 = equation[i].token.variable;
					cnt = 1;
				} else if (equation[i].token.variable == v1) {
					cnt++;
				}
			}
		}
		if (v1 == -1)
			break;
		last_v = v1;
		if (v1 > IMAGINARY) {	/* ignore constant variables */
			va[vc].v = v1;
			va[vc].count = cnt;
			vc++;
		}
	}
	if (vc) {
		/* sort variable array va[] */
		qsort((char *) va, vc, sizeof(*va), simpb_vcmp);
		/* factor division by most frequently occurring variables first */
		simp2_divide(equation, np, va[0].v, fc_level);
		for (i = 1; i < vc; i++) {
			if (factor_divide(equation, np, va[i].v, 0.0))
				simp2_divide(equation, np, va[i].v, fc_level);
		}
		simp2_divide(equation, np, 0L, fc_level);
		/* factor all sub-expressions in order of most frequently occurring variables */
		for (i = 0; i < vc; i++) {
			while (factor_plus(equation, np, va[i].v, 0.0)) {
				simp2_divide(equation, np, 0L, fc_level);
			}
		}
	}
#if	0	/* Messes up support/gcd.in and tests/quartic.in */
	make_simple_fractions(equation, np);
#endif
	/* make sure equation side is completely factored */
	while (factor_divide(equation, np, MATCH_ANY, 0.0)) {
		simp2_divide(equation, np, MATCH_ANY, fc_level);
	}
	while (factor_plus(equation, np, MATCH_ANY, 0.0)) {
		simp2_divide(equation, np, 0L, fc_level);
	}
	simp_ssub(equation, np, MATCH_ANY, 0.0, power_flag, true, fc_level);
#if	0	/* Seems to complicate and change constants slightly when enabled. */
	if (power_flag) {
		make_simple_fractions(equation, np);
		factor_power(equation, np);
		simp_ssub(equation, np, MATCH_ANY, 0.0, power_flag, true, fc_level);
	}
#endif
}

/*
 * Convert expressions with any algebraic fractions into a single simple fraction.
 * Used by the fraction command.
 *
 * Globals tlhs[] and trhs[] are wiped out.
 */
void
simple_frac_side(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to length of equation side */
{
	if (*np == 1) {
		make_simple_fractions(equation, np);
		fractions_and_group(equation, np);
		return;
	}
	simp_loop(equation, np);
	poly_factor(equation, np, true);
	do {
		do {
			do {
				simp_ssub(equation, np, 0L, 0.0, false, true, 5);
			} while (poly_gcd_simp(equation, np));
		} while (uf_power(equation, np));
	} while (super_factor(equation, np, 3));
	side_debug(2, equation, *np);

	make_simple_fractions(equation, np);
	uf_tsimp(equation, np);
#if	0	/* Not really needed, but makes it end up the same as the simplify command. */
	make_simple_fractions(equation, np);
	uf_power(equation, np);
	integer_root_simp(equation, np);
	simpb_side(equation, np, true, true, 3);
#endif
	poly_factor(equation, np, true);
	simpb_side(equation, np, true, false, 2);
	simpb_side(equation, np, true, false, 2);	/* Added for thoroughness, making sure everything is uf_power()ed. */
	fractions_and_group(equation, np);
}

/*
 * This is the slow and thorough simplify of the simplify command.
 * Applies many equivalent algebraic transformations and their inverses (like unfactor and factor),
 * then does generalized polynomial simplifications.
 *
 * Globals tlhs[] and trhs[] are wiped out.
 */
void
simpa_side(equation, np, quick_flag, frac_flag)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of the equation side */
int		quick_flag;	/* "simplify quick" option, simpler with no (x+1)^2 expansion */
int		frac_flag;	/* "simplify fraction" option, simplify to the ratio of two polynomials */
{
	int		i;
	int		flag, poly_flag = true;
	jmp_buf		save_save;

	if (*np == 1) {	/* no need to full simplify a single constant or variable */
		make_simple_fractions(equation, np);
		simpb_side(equation, np, true, !frac_flag, 2);
		return;
	}
	debug_string(2, "Simplify input:");
	side_debug(2, equation, *np);
	simp_loop(equation, np);
#if	1
	do {
		simp_ssub(equation, np, 0L, 1.0, false, true, 5);
	} while (uf_power(equation, np));
	while (factor_power(equation, np)) {
		simp_loop(equation, np);
	}
#else
	simpb_side(equation, np, false, true, 5);
#endif
	if (rationalize_denominators) {
		rationalize(equation, np);
	}
	unsimp_power(equation, np);
	uf_tsimp(equation, np);

/* Here is the only place in Mathomatic that we do complete modulus (%) simplification: */
	uf_pplus(equation, np);
	uf_repeat(equation, np);
	do {
		elim_loop(equation, np);
	} while (mod_simp(equation, np));

/* Here we try to simplify out unnecessary negative constants and imaginary numbers: */
	simp_i(equation, np);
	unsimp_power(equation, np);
	uf_times(equation, np);
	simp_ssub(equation, np, 0L, 1.0, true, true, 5);
	unsimp_power(equation, np);
	uf_neg_help(equation, np);
	uf_tsimp(equation, np);
	do {
		do {
			simp_ssub(equation, np, 0L, 1.0, false, true, 6);
		} while (uf_power(equation, np));
	} while (!quick_flag && super_factor(equation, np, 2));
	if (poly_gcd_simp(equation, np)) {
		simp_ssub(equation, np, 0L, 1.0, false, true, 6);
	}
	side_debug(2, equation, *np);
	unsimp_power(equation, np);
	uf_times(equation, np);
	factorv(equation, np, IMAGINARY);
	uf_pplus(equation, np);
	simp_ssub(equation, np, 0L, 1.0, true, false, 5);
	if (poly_gcd_simp(equation, np)) {
		factorv(equation, np, IMAGINARY);
		uf_pplus(equation, np);
		simp_ssub(equation, np, 0L, 1.0, true, false, 5);
	}
	uf_times(equation, np);
	uf_pplus(equation, np);
	factor_imaginary(equation, np);
	uf_power(equation, np);
	do {
		do {
			simp_ssub(equation, np, 0L, 1.0, false, true, 6);
		} while (uf_power(equation, np));
	} while (!quick_flag && super_factor(equation, np, 2));

/* Here we do the greatest expansion; if it fails, do less expansion. */
	partial_flag = frac_flag;
	n_tlhs = *np;
	blt(tlhs, equation, n_tlhs * sizeof(token_type));
	blt(save_save, jmp_save, sizeof(jmp_save));
	if ((i = setjmp(jmp_save)) != 0) {	/* trap errors */
		blt(jmp_save, save_save, sizeof(jmp_save));
		if (i == 13) {	/* critical error code */
			longjmp(jmp_save, i);
		}
		/* an error occurred, restore the original expression */
		*np = n_tlhs;
		blt(equation, tlhs, n_tlhs * sizeof(token_type));
		if (i == 14) {
			debug_string(1, "Simplify not expanding fully, due to oversized expression.");
		} else {
			debug_string(0, "Simplify not expanding fully, due to some error.");
		}
		partial_flag = true;	/* expand less */
		uf_tsimp(equation, np);
	} else {
		if (quick_flag) {
			uf_tsimp(equation, np);
		} else {
			/* expand powers of 2 and higher, might result in error_huge() trap */
			do {
				uf_power(equation, np);
				uf_repeat(equation, np);
			} while (uf_tsimp(equation, np));
		}
		blt(jmp_save, save_save, sizeof(jmp_save));
	}
	partial_flag = true;

	simpb_side(equation, np, true, true, 2);
	debug_string(1, "Simplify result before applying polynomial operations:");
	side_debug(1, equation, *np);
	for (flag = false;;) {
		/* divide top and bottom of fractions by any polynomial GCD found */
		if (poly_gcd_simp(equation, np)) {
			flag = false;
			simpb_side(equation, np, false, true, 3);
		}
		/* factor polynomials */
		if (!flag && poly_factor(equation, np, true)) {
			flag = true;
			simpb_side(equation, np, false, true, 3);
			continue;
		}
		/* simplify algebraic fractions with polynomial and smart division */
		if (!frac_flag && div_remainder(equation, np, poly_flag, quick_flag)) {
			flag = false;
			simpb_side(equation, np, false, true, 3);
			continue;
		}
		break;
	}
	debug_string(2, "Raw simplify result after applying polynomial operations:");
	side_debug(2, equation, *np);
	simp_constant_power(equation, np);
	simp_ssub(equation, np, 0L, 1.0, true, true, 5);
	unsimp_power(equation, np);
	make_simple_fractions(equation, np);
	factor_power(equation, np);
	uf_tsimp(equation, np);
	make_simple_fractions(equation, np);
	uf_power(equation, np);
	integer_root_simp(equation, np);
	simpb_side(equation, np, true, true, 3);
	poly_factor(equation, np, true);
	simpb_side(equation, np, true, !frac_flag, 2);
}

/*
 * This routine is used by the simplify command,
 * and is the slowest and most thorough simplify of all.
 * It repeats the simplification until the smallest expression is achieved,
 * if repeat_flag is true.
 *
 * Globals tes[], tlhs[], and trhs[] are wiped out.
 */
void
simpa_repeat_side(equation, np, quick_flag, frac_flag)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of the equation side */
int		quick_flag;	/* "simplify quick" option, simpler fractions with no (x+1)^2 expansion */
int		frac_flag;	/* "simplify fraction" option, simplify to the ratio of two polynomials */
{
	if (*np <= 0)
		return;
	simpa_side(equation, np, quick_flag, frac_flag);
	if (repeat_flag && *np > 1) {
		do {
			n_tes = *np;
			blt(tes, equation, n_tes * sizeof(token_type));
			simpa_side(equation, np, quick_flag, frac_flag);
		} while (*np < n_tes);
		if (*np != n_tes) {
			*np = n_tes;
			blt(equation, tes, n_tes * sizeof(token_type));
		}
	}
}

/*
 * Completely and repeatedly (if repeat_flag) simplify an equation space to the smallest possible size.
 *
 * Globals tes[], tlhs[], and trhs[] are clobbered.
 */
void
simpa_repeat(n, quick_flag, frac_flag)
int	n;		/* equation space number to simplify */
int	quick_flag;	/* "simplify quick" option, simpler fractions with no (x+1)^2 expansion */
int	frac_flag;	/* "simplify fraction" option, simplify to the ratio of two polynomials */
{
	if (empty_equation_space(n))
		return;
	simpa_repeat_side(lhs[n], &n_lhs[n], quick_flag, frac_flag);
	if (n_rhs[n] > 0) {
		simpa_repeat_side(rhs[n], &n_rhs[n], quick_flag, frac_flag);
	}
}

/*
 * This routine is used by the fraction command.
 * It repeats the simplification until the smallest expression is achieved,
 * if repeat_flag is true.
 *
 * Globals tes[], tlhs[], and trhs[] are wiped out.
 */
void
simple_frac_repeat_side(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of the equation side */
{
	if (*np <= 0)
		return;
	simple_frac_side(equation, np);
	if (repeat_flag) {
		do {
			n_tes = *np;
			blt(tes, equation, n_tes * sizeof(token_type));
			simple_frac_side(equation, np);
		} while (*np < n_tes);
		if (*np != n_tes) {
			*np = n_tes;
			blt(equation, tes, n_tes * sizeof(token_type));
		}
	}
}

/*
 * Commonly used quick simplify routine that doesn't factor.
 *
 * Return true if factor_times() did something.
 */
int
simp_loop(equation, np)
token_type	*equation;	/* pointer to the beginning of equation side to simplify */
int		*np;		/* pointer to length of equation side */
{
	int	i;
	int	rv = false;

	do {
		do {
			do {
				do {
					elim_loop(equation, np);
				} while (simp2_power(equation, np));
				i = factor_times(equation, np);
				if (i)
					rv = true;
			} while (i);
		} while (elim_sign(equation, np));
	} while (subtract_itself(equation, np));
	return rv;
}

/*
 * Convert (x^n)^m to x^(n*m) when appropriate.
 * Fixed to work the same as Maxima when symb_flag is false,
 * otherwise always converts (x^n)^m to x^(n*m), which sometimes simplifies more.
 *
 * Earlier versions of Mathomatic always converted (x^n)^m to x^(n*m),
 * which is the wrong thing to do, because they are not always equivalent.
 * That is now the job of the "simplify symbolic" command, which sets symb_flag to true.
 *
 * Return true if equation side was modified.
 */
int
simp_pp(equation, np)
token_type	*equation;
int		*np;
{
	int		i, j, k;
	int		ilevel, jlevel;
	int		modified = false;
	double		numerator, denominator;

	for (i = 1; i < *np; i += 2) {
#if	DEBUG
		if (equation[i].kind != OPERATOR) {
			error_bug("Bug found in simp_pp(), operators are misplaced.");
		}
#endif
		if (equation[i].token.operatr != POWER)
			continue;
		ilevel = equation[i].level;
		for (j = i + 2; j < *np; j += 2) {
			jlevel = equation[j].level;
			if (jlevel == ilevel - 1 && equation[j].token.operatr == POWER) {
				if (!symb_flag && (equation[i-1].level != ilevel || equation[i-1].kind != CONSTANT || equation[i-1].token.constant < 0)) {
					if (jlevel == equation[j+1].level && equation[j+1].kind == CONSTANT) {
						f_to_fraction(equation[j+1].token.constant, &numerator, &denominator);
						if (fmod(denominator, 2.0) == 0.0) {
							if ((i + 2) == j && equation[i+1].kind == CONSTANT) {
								f_to_fraction(equation[i+1].token.constant, &numerator, &denominator);
								if (fmod(numerator, 2.0) == 0.0) {
									break;
								}
							} else
								break;
						}
					} else {
						if ((i + 2) == j && equation[i+1].kind == CONSTANT) {
							f_to_fraction(equation[i+1].token.constant, &numerator, &denominator);
							if (fmod(numerator, 2.0) == 0.0) {
								break;
							}
						} else
							break;
					}
				}
				equation[j].token.operatr = TIMES;
				for (k = j; k < *np && equation[k].level >= jlevel; k++) {
					equation[k].level += 2;
				}
				for (k = i + 1; k < j; k++)
					equation[k].level++;
				i -= 2;
				modified = true;
				break;
			}
			if (jlevel <= ilevel)
				break;
		}
	}
	return modified;
}

/*
 * Simplify surds like 12^(1/2) to 2*3^(1/2).
 *
 * Return true if equation side was modified.
 */
int
integer_root_simp(equation, np)
token_type	*equation;
int		*np;
{
	int	modified = false;
	int	i, j;
	int	level;
	double	d1, d2, numerator, denominator;

	for (i = 1; (i + 3) < *np; i += 2) {
#if	DEBUG
		if (equation[i].kind != OPERATOR) {
			error_bug("Bug found in integer_root_simp(), operators are misplaced.");
		}
#endif
		if (equation[i].token.operatr == POWER) {
			level = equation[i].level;
			if (equation[i-1].level == level
			    && equation[i+1].level == level + 1
			    && equation[i+2].level == level + 1
			    && equation[i+3].level == level + 1
			    && equation[i+2].token.operatr == DIVIDE
			    && equation[i-1].kind == CONSTANT
			    && equation[i+1].kind == CONSTANT
			    && equation[i+3].kind == CONSTANT) {
				if ((i + 4) < *np && equation[i+4].level >= level)
					continue;
				numerator = equation[i+1].token.constant;
				if (numerator > 50.0 || numerator < 1.0 || fmod(numerator, 1.0) != 0.0)
					continue;
				denominator = equation[i+3].token.constant;
				if (denominator > 50.0 || denominator < 2.0 || fmod(denominator, 1.0) != 0.0)
					continue;
				errno = 0;
				d2 = pow(equation[i-1].token.constant, numerator);
				if (errno) {
					continue;
				}
				if (!factor_one(d2))
					continue;
				d1 = 1.0;
				for (j = 0; j < uno; j++) {
					if (unique[j] > 0.0) {
						while (ucnt[j] >= denominator) {
							d1 *= unique[j];
							ucnt[j] -= denominator;
						}
					}
				}
				if (d1 == 1.0)
					continue;
				if (*np + 2 > n_tokens) {
					error_huge();
				}
				equation[i+1].token.constant = 1.0;
				equation[i-1].token.constant = multiply_out_unique();
				for (j = i - 1; j < (i + 4); j++) {
					equation[j].level++;
				}
				blt(&equation[i+1], &equation[i-1], (*np - (i - 1)) * sizeof(token_type));
				*np += 2;
				equation[i-1].level = level;
				equation[i-1].kind = CONSTANT;
				equation[i-1].token.constant = d1;
				equation[i].level = level;
				equation[i].kind = OPERATOR;
				equation[i].token.operatr = TIMES;
				modified = true;
				i += 4;
			}
		}
	}
	return modified;
}

/*
 * Simplify constant^(constant*x) to (constant^constant)^x.
 * I am not sure if this is a good thing, because it changes the base of the exponent.
 * The tests don't require this routine.
 *
 * Return true if equation side was modified.
 */
int
simp_constant_power(equation, np)
token_type	*equation;
int		*np;
{
	int	i, j;
	int	level;
	int	modified = false;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr != POWER) {
			continue;
		}
		level = equation[i].level;
		if (equation[i-1].level != level || equation[i-1].kind != CONSTANT)
			continue;
		if (equation[i-1].token.constant < 0 && !symb_flag)
			continue;
		if (equation[i+1].level != level + 1 || equation[i+1].kind != CONSTANT
		    || equation[i+1].token.constant == 1.0)
			continue;
		j = i + 2;
		if (j >= *np || equation[j].level != level + 1)
			continue;
		switch (equation[j].token.operatr) {
		case TIMES:
			break;
		case DIVIDE:
			if (*np + 2 > n_tokens) {
				error_huge();
			}
			blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
			*np += 2;
			equation[j+1].level = level + 1;
			equation[j+1].kind = CONSTANT;
			equation[j+1].token.constant = 1.0;
			break;
		default:
			continue;
		}
		equation[j].level = level;
		equation[j].token.operatr = POWER;
		equation[i-1].level++;
		equation[i].level++;
		modified = true;
	}
	return modified;
}

/*
 * Convert x^-y to 1/(x^y).
 *
 * Return true if equation side was modified.
 */
int
simp2_power(equation, np)
token_type	*equation;
int		*np;
{
	int	i, i1, j, k;
	int	level;
	int	op;
	int	modified = false;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr == POWER) {
			level = equation[i].level;
			op = 0;
			k = -1;
			for (j = i + 1; j < *np && equation[j].level >= level; j++) {
				if (equation[j].level == level + 1) {
					if (equation[j].kind == OPERATOR) {
						op = equation[j].token.operatr;
					} else if (equation[j].kind == CONSTANT && equation[j].token.constant < 0.0) {
						k = j;
					}
				}
			}
			if (j - i <= 2 && equation[i+1].kind == CONSTANT && equation[i+1].token.constant < 0.0) {
				k = i + 1;
			} else if (k < 0)
				continue;
			switch (op) {
			case 0:
			case TIMES:
			case DIVIDE:
				if (*np + 2 > n_tokens) {
					error_huge();
				}
				equation[k].token.constant = -equation[k].token.constant;
				for (k = i - 2;; k--) {
					if (k < 0 || equation[k].level < level)
						break;
				}
				k++;
				for (i1 = k; i1 < j; i1++)
					equation[i1].level++;
				blt(&equation[k+2], &equation[k], (*np - k) * sizeof(token_type));
				*np += 2;
				equation[k].level = level;
				equation[k].kind = CONSTANT;
				equation[k].token.constant = 1.0;
				k++;
				equation[k].level = level;
				equation[k].kind = OPERATOR;
				equation[k].token.operatr = DIVIDE;
				modified = true;
			}
		}
	}
	return modified;
}

/*
 * fmod(3) in the standard C math library has some problems.
 * Hopefully this fixes them.
 */
double
fixed_fmod(k1, k2)
double	k1, k2;
{
	double	d;

	if (k2 == 0 || !isfinite(k1) || !isfinite(k2) || (fmod(k1, 1.0) == 0 && fmod(k2, 1.0) == 0)) {
		k1 = fmod(k1, k2);
	} else {
		/* fmod() doesn't work well with fractional amounts. */
		/* Another way to get the remainder of division: */
		k1 = modf(k1 / k2, &d) * k2;
	}
	return k1;
}

/*
 * Combine two or more constants on the same level of parentheses.
 * If "iflag" is false, don't produce imaginary numbers.
 *
 * Return true if equation side was modified.
 */
int
combine_constants(equation, np, iflag)
token_type	*equation;	/* pointer to the beginning of equation side */
int		*np;		/* pointer to length of equation side */
int		iflag;		/* produce imaginary numbers flag */
{
	return const_recurse(equation, np, 0, 1, iflag);
}

/*
 * Do the floating point arithmetic for Mathomatic.
 *
 * Return true if successful.
 * domain_check must be set to false after this.
 */
int
calc(op1p, k1p, op2, k2)
int	*op1p;	/* Pointer to operator 1, which comes immediately before operand 1 if this operator exists. */
double	*k1p;	/* Pointer to operand 1 and where to store the result of the calculation. */
int	op2;	/* Operator 2; always exists and comes immediately before operand 2, and usually after operand 1. */
double	k2;	/* Operand 2; ignored for unary operators. */
{
#if	_REENTRANT
	int	sign = 1;
#endif
	int	op1;
	double	d, d1, d2;

	domain_check = false;
	errno = 0;
	if (op1p) {
		op1 = *op1p;
	} else {
		op1 = 0;
	}
	switch (op2) {
	case PLUS:
	case MINUS:
		if (op1 == MINUS)
			d = -(*k1p);
		else
			d = *k1p;
		d1 = fabs(d) * epsilon;
		if (op2 == PLUS) {
			d += k2;
		} else {
			d -= k2;
		}
		if (fabs(d) < d1)
			d = 0.0;
		if (op1 == 0) {
			*k1p = d;
		} else {
			if (d >= 0.0) {
				*op1p = PLUS;
				*k1p = d;
			} else {
				*op1p = MINUS;
				*k1p = -d;
			}
		}
		break;
	case TIMES:
	case DIVIDE:
		if (op1 == 0)
			op1 = TIMES;
		if (op1 == op2) {
			*k1p *= k2;
		} else {
			if (op1 == DIVIDE) {
				check_divide_by_zero(*k1p);
				*k1p = k2 / *k1p;
				*op1p = TIMES;
			} else if (op2 == DIVIDE) {
				check_divide_by_zero(k2);
				*k1p = *k1p / k2;
			}
		}
		break;
	case IDIVIDE:
		check_divide_by_zero(k2);
		modf(*k1p / k2, k1p);
		break;
	case MODULUS:
		if (k2 == 0) {
			warning(_("Modulo 0 encountered."));
		}
		*k1p = fixed_fmod(*k1p, k2);
		if (modulus_mode && *k1p < 0.0) {
			*k1p += fabs(k2);	/* make positive */
		}
		if (modulus_mode == 1 && k2 < 0.0 && *k1p > 0.0) {
			*k1p += k2;		/* make negative */
		}
		/* modulus_mode == 0 result same sign as dividend,
		   modulus_mode == 1 result same sign as divisor,
                   modulus_mode == 2 result is always positive or zero */
		break;
	case POWER:
		if (*k1p < 0.0 && fmod(k2, 1.0) != 0.0) {
			/* it's probably imaginary; pow() will give a domain error, so skip these calculations */
			break;
		}
		domain_check = true;
		if (*k1p == 0.0 && k2 == 0.0) {
			warning(_("0^0 encountered, might be considered indeterminate."));
			d = 1.0;	/* 0^0 = 1 */
		} else if (*k1p == 0 && k2 < 0.0) {
			warning(_("Divide by zero (0 raised to negative power)."));
			d = INFINITY;
		} else {
			d = pow(*k1p, k2);
			if (preserve_surds && !approximate_roots) {
				if (isfinite(k2) && fmod(k2, 1.0) != 0.0 && f_to_fraction(*k1p, &d1, &d2)) {
					if (!f_to_fraction(d, &d1, &d2)) {
						domain_check = false;
						return false;
					}
				}
			}
		}
#if	1
		if (errno == ERANGE) {	/* preserve overflowed powers rather than aborting with an error message */
			domain_check = false;
			return false;
		}
#endif
		check_err();
		if (domain_check)
			*k1p = d;
		break;
	case FACTORIAL:
#if	NOGAMMA
#warning "Using integer factorial only."
		/* calculate integer factorial if gamma() functions don't exist */
		if (*k1p > 170.0 || *k1p < 0.0 || fmod(*k1p, 1.0) != 0.0) {
			return false;
		}
		d = 1.0;
		for (d1 = 2.0; d1 <= *k1p; d1 += 1.0) {
			d *= d1;
		}
#else
#if	_XOPEN_SOURCE >= 600 || _ISOC99_SOURCE || __USE_ISOC99 || MINGW || __APPLE__ || USE_TGAMMA
		/* Use true gamma for factorial if available, it is the nicest gamma() function. */
		/* Problem is I don't know how to properly tell if it is available. */
		d = tgamma(*k1p + 1.0);
		if (errno) {	/* don't evaluate if overflow */
			return false;
		}
#else
#if	!_REENTRANT
#warning "Using lgamma(3); Not a problem, but tgamma(3) is more direct, if available."
		d = exp(lgamma(*k1p + 1.0)) * signgam;
#else	/* use re-entrant version: */
#warning "Using lgamma_r(3); Not a problem, but tgamma(3) is more direct, if available."
		d = exp(lgamma_r(*k1p + 1.0, &sign));
		d *= sign;
#endif
/* Just compile with -DUSE_TGAMMA, if tgamma(3) is available and not being used. */
		if (errno) {	/* don't evaluate if overflow */
			return false;
		}
#endif
#endif
		*k1p = d;
		break;
	default:
		return false;
	}
	return true;
}

static int
const_recurse(equation, np, loc, level, iflag)
token_type	*equation;
int		*np, loc, level, iflag;
{
	int		loc1, old_loc;
	int		const_count = 0;
	int		op;
	int		modified = false;
	double		d1, d2, d3, numerator, denominator;
	complexs	cv, p;

	loc1 = old_loc = loc;
	for (;; loc++) {
beginning:
		if (loc >= *np || equation[loc].level < level) {
			if (loc - old_loc == 1)	/* decrement the level of parentheses if only one constant left */
				equation[old_loc].level = max(level - 1, 1);
			return modified;
		}
		if (equation[loc].level > level) {
			modified |= const_recurse(equation, np, loc, level + 1, iflag);
			for (; loc < *np && equation[loc].level > level; loc++)
				;
			goto beginning;
		}
		if (equation[loc].kind == CONSTANT) {
			if (const_count == 0) {
				loc1 = loc;
				const_count++;
				continue;
			}
			op = equation[loc-1].token.operatr;
			d1 = equation[loc1].token.constant;
			d2 = equation[loc].token.constant;
			if (calc((loc1 <= old_loc) ? NULL : &equation[loc1-1].token.operatr, &d1, op, d2)) {
				if (op == POWER && !domain_check) {
					if (!f_to_fraction(d2, &numerator, &denominator)) {	/* if irrational power */
						if (!iflag || (preserve_surds && !approximate_roots))
							return modified;
						cv.re = d1;
						cv.im = 0.0;
						p.re = d2;
						p.im = 0.0;
						cv = complex_pow(cv, p);
						if (*np + 2 > n_tokens) {
							error_huge();
						}
						blt(&equation[loc1+2], &equation[loc1], (*np - loc1) * sizeof(token_type));
						*np += 2;
						equation[loc1].level = level;
						equation[loc1].kind = CONSTANT;
						equation[loc1].token.constant = cv.re;
						loc1++;
						equation[loc1].level = level;
						equation[loc1].kind = OPERATOR;
						equation[loc1].token.operatr = PLUS;
						level++;
						equation[loc].level = level;
						equation[loc].kind = VARIABLE;
						equation[loc].token.variable = IMAGINARY;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = OPERATOR;
						equation[loc].token.operatr = TIMES;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = CONSTANT;
						equation[loc].token.constant = cv.im;
						return true;
					}
					errno = 0;
					d3 = pow(-d1, d2);
					check_err();
					if (!always_positive(denominator)) {
						if (*np + 2 > n_tokens) {
							error_huge();
						}
						blt(&equation[loc1+2], &equation[loc1], (*np - loc1) * sizeof(token_type));
						*np += 2;
						equation[loc1].level = level + 1;
						equation[loc1].kind = CONSTANT;
						equation[loc1].token.constant = -d1;
						loc1++;
						equation[loc1].level = level + 1;
						equation[loc1].kind = OPERATOR;
						equation[loc1].token.operatr = POWER;
						equation[loc].level = level + 1;
						equation[loc].kind = CONSTANT;
						equation[loc].token.constant = d2;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = OPERATOR;
						equation[loc].token.operatr = TIMES;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = CONSTANT;
						if (always_positive(numerator)) {
							equation[loc].token.constant = 1.0;
						} else {
							equation[loc].token.constant = -1.0;
						}
						return true;
					}
					if (!iflag)
						return modified;
					if (*np + 2 > n_tokens) {
						error_huge();
					}
					blt(&equation[loc1+2], &equation[loc1], (*np - loc1) * sizeof(token_type));
					*np += 2;
					if (d2 == 0.5) {
						equation[loc1].level = level + 1;
						equation[loc1].kind = CONSTANT;
						equation[loc1].token.constant = -d1;
						loc1++;
						equation[loc1].level = level + 1;
						equation[loc1].kind = OPERATOR;
						equation[loc1].token.operatr = POWER;
						equation[loc].level = level + 1;
						equation[loc].kind = CONSTANT;
						equation[loc].token.constant = d2;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = OPERATOR;
						equation[loc].token.operatr = TIMES;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = VARIABLE;
						equation[loc].token.variable = IMAGINARY;
					} else {
						equation[loc1].level = level;
						equation[loc1].kind = CONSTANT;
						equation[loc1].token.constant = d3;
						loc1++;
						equation[loc1].level = level;
						equation[loc1].kind = OPERATOR;
						equation[loc1].token.operatr = TIMES;
						level++;
						equation[loc].level = level;
						equation[loc].kind = VARIABLE;
						equation[loc].token.variable = IMAGINARY;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = OPERATOR;
						equation[loc].token.operatr = POWER;
						loc++;
						equation[loc].level = level;
						equation[loc].kind = CONSTANT;
						equation[loc].token.constant = d2 * 2.0;
					}
					return true;
				} else {
					equation[loc1].token.constant = d1;
					modified = true;
					domain_check = false;
					blt(&equation[loc-1], &equation[loc+1], (*np - (loc + 1)) * sizeof(token_type));
					*np -= 2;
					loc -= 2;
				}
			} else {
				domain_check = false;
			}
		}
	}
}

/*
 * Eliminate or fix operations involving constants that can be simplified or normalized.
 *
 * Return true if equation side was significantly modified.
 */
int
elim_k(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
	token_type	*p1, *p2, *p3, *p4;
	token_type	*ep;			/* end pointer */
	int		modified = false;
	int		level;
	double		d, numerator, denominator;
	int		flag;

	for (p1 = equation + 1;;) {
		ep = &equation[*np];
		if (p1 >= ep)
			break;
		if (p1->kind != OPERATOR) {
			p1++;
			continue;
		}
		level = p1->level;
		switch (p1->token.operatr) {
		case PLUS:
		case MINUS:
/* Fix addition/subtraction of negative, zero, or infinity constants. */
			p2 = p1 + 1;
			if (p1 + 2 < ep && (p1 + 2)->level == level + 1
			    && ((p1 + 2)->token.operatr == TIMES || (p1 + 2)->token.operatr == DIVIDE)
			    && p2->kind == CONSTANT && p2->token.constant < 0.0) {
				if (p1->token.operatr == PLUS)
					p1->token.operatr = MINUS;
				else
					p1->token.operatr = PLUS;
				p2->token.constant = -(p2->token.constant);
			}
			if (p2->level == level && p2->kind == CONSTANT) {
				if (p2->token.constant < 0.0) {
					if (p1->token.operatr == PLUS)
						p1->token.operatr = MINUS;
					else
						p1->token.operatr = PLUS;
					p2->token.constant = -p2->token.constant;
				}
				if (p2->token.constant == 0.0) {
					blt(p1, p1 + 2, (char *) ep - (char *) (p1 + 2));
					*np -= 2;
					modified = true;
					continue;
				}
			}
			if ((p1 - 1)->level == level && (p1 - 1)->kind == CONSTANT && isinf((p1 - 1)->token.constant)) {
				p2 = p1 - 1;
			}
			if (p2->level == level && p2->kind == CONSTANT && isinf(p2->token.constant)) {
				flag = false;
				for (p3 = p1;; p3--) {
					if (p3->level < level) {
						p3++;
						break;
					}
					if (p3->kind == CONSTANT && p3 != p2 && !isfinite(p3->token.constant)) {
						flag = true;
					}
					if (p3 == equation)
						break;
				}
				for (p4 = p1; p4 < ep && p4->level >= level; p4++) {
					if (p4->kind == CONSTANT && p4 != p2 && !isfinite(p4->token.constant)) {
						flag = true;
					}
				}
				if (!flag) { /* no other infinities on level */
					if (p2 > p3 && (p2 - 1)->token.operatr == MINUS) {
						p2->token.constant = -(p2->token.constant);
					}
					blt(p2 + 1, p4, (char *) ep - (char *) p4);
					*np -= p4 - (p2 + 1);
					ep = &equation[*np];
					blt(p3, p2, (char *) ep - (char *) p2);
					*np -= p2 - p3;
					return true;
				}
			}
			break;
		}
		p2 = p1 - 1;
		switch (p1->token.operatr) {
		case PLUS:
/* remove 0+ */
			if (p2->level == level && p2->kind == CONSTANT && p2->token.constant == 0.0) {
				blt(p2, p1 + 1, (char *) ep - (char *) (p1 + 1));
				*np -= 2;
				modified = true;
				continue;
			}
			break;
		case MINUS:
/* 0-x to (-1*x) */
			if (p2->level == level && p2->kind == CONSTANT && p2->token.constant == 0.0) {
				if (p2 == equation || (p2 - 1)->level < level) {
					p2->token.constant = -1.0;
					p1->token.operatr = TIMES;
					binary_parenthesize(equation, *np, p1 - equation);
					modified = true;
					continue;
				}
			}
			break;
		case TIMES:
			if (p2->level == level && p2->kind == CONSTANT) {
				if (p2->token.constant == 0.0) {
/* Replace 0*x with 0. */
					for (p2 = p1 + 2; p2 < ep; p2 += 2) {
						if (p2->level < level)
							break;
					}
					blt(p1, p2, (char *) ep - (char *) p2);
					*np -= p2 - p1;
					modified = true;
					continue;
				}
				if (fabs(p2->token.constant - 1.0) <= epsilon) {
/* Replace 1*x with x. */
					blt(p2, p1 + 1, (char *) ep - (char *) (p1 + 1));
					*np -= 2;
					modified = true;
					continue;
				}
			}
			if ((p1 + 1)->level == level && (p1 + 1)->kind == CONSTANT) {
/* Move any constant to the beginning of a multiplicative sub-expression. */
				d = (p1 + 1)->token.constant;
				for (p2 = p1 - 1; p2 > equation; p2--) {
					if ((p2 - 1)->level < level)
						break;
				}
				if (p2->level == level && p2->kind == CONSTANT) {
					/* already a constant at the beginning, so don't move anything */
					break;
				}
				blt(p2 + 2, p2, (char *) p1 - (char *) p2);
				p2->level = level;
				p2->kind = CONSTANT;
				p2->token.constant = d;
				(p2 + 1)->level = level;
				(p2 + 1)->kind = OPERATOR;
				(p2 + 1)->token.operatr = TIMES;
				if (p2 > equation) {
					p1 = p2 - 1;
				} else {
					p1 = equation + 1;
				}
				continue;
			}
			break;
		case DIVIDE:
			if (p2->level == level && p2->kind == CONSTANT && p2->token.constant == 0.0) {
/* Replace 0/x with 0. */
				for (p2 = p1 + 2; p2 < ep; p2 += 2) {
					if (p2->level < level)
						break;
				}
				blt(p1, p2, (char *) ep - (char *) p2);
				*np -= p2 - p1;
				modified = true;
				continue;
			}
			p2 = p1 + 1;
			if (p2->level == level && p2->kind == CONSTANT) {
/* Replace division by a constant with times its reciprocal. */
				f_to_fraction(p2->token.constant, &numerator, &denominator);
				check_divide_by_zero(numerator);
				p2->token.constant = denominator / numerator;
				p1->token.operatr = TIMES;
				continue;
			}
			if (p2->level == level && p2->kind == VARIABLE && (p2->token.variable & VAR_MASK) == SIGN) {
/* Replace division by a sign variable with times the sign variable. */
				p1->token.operatr = TIMES;
				continue;
			}
			break;
		case MODULUS:
		case IDIVIDE:
			if (p2->level == level && p2->kind == CONSTANT && p2->token.constant == 0.0) {
/* Replace 0%x with 0. */
				for (p2 = p1 + 2; p2 < ep; p2 += 2) {
					if (p2->level < level)
						break;
				}
				blt(p1, p2, (char *) ep - (char *) p2);
				*np -= p2 - p1;
				modified = true;
				continue;
			}
			if (p1->token.operatr == MODULUS) {
				if ((p1 + 1)->level == level && (p1 + 1)->kind == CONSTANT) {
					d = fabs((p1 + 1)->token.constant);
					if (d > epsilon && fmod(1.0 / d, 1.0) == 0.0) {
						for (p2 = p1 - 1; p2 > equation; p2--) {
							if ((p2 - 1)->level < level)
								break;
						}
						if (is_integer_expr(p2, p1 - p2)) {
							blt(p2, p1 + 1, (char *) ep - (char *) (p1 + 1));
							*np -= (p1 + 1) - p2;
							p2->token.constant = 0.0;
							if (p2 > equation) {
								p1 = p2 - 1;
							} else {
								p1 = equation + 1;
							}
							modified = true;
							continue;
						}
					}
				}
			}
			break;
		case POWER:
			if (p2->level == level && p2->kind == CONSTANT) {
				if (p2->token.constant == 1.0) {
/* Replace 1^x with 1. */
					for (p2 = p1 + 2; p2 < ep; p2 += 2) {
						if (p2->level <= level)
							break;
					}
					blt(p1, p2, (char *) ep - (char *) p2);
					*np -= p2 - p1;
					modified = true;
					continue;
				}
			}
			p2 = p1 + 1;
			if (p2->level == level && p2->kind == CONSTANT) {
				if (p2->token.constant == 0.0) {
/* Replace x^0 with 1. */
					for (p2 = p1 - 1; p2 > equation; p2--) {
						if ((p2 - 1)->level <= level)
							break;
					}
					blt(p2, p1 + 1, (char *) ep - (char *) (p1 + 1));
					*np -= (p1 + 1) - p2;
					p2->token.constant = 1.0;
					p1 = p2 + 1;
					modified = true;
					continue;
				}
				if (fabs(p2->token.constant - 1.0) <= epsilon) {
/* Replace x^1 with x. */
					blt(p1, p1 + 2, (char *) ep - (char *) (p1 + 2));
					*np -= 2;
					modified = true;
					continue;
				}
			}
			break;
		}
		p1 += 2;
	}
	return modified;
}

/*
 * Compare two sub-expressions for equality.
 * This is quick and low level and does not simplify or modify the expressions.
 *
 * If equal, return true with *diff_signp = 0.
 * if p1 * -1 == p2, return true with *diff_signp = 1.
 * Otherwise return false.
 */
int
se_compare(p1, n1, p2, n2, diff_signp)
token_type	*p1;		/* first sub-expression pointer */
int		n1;		/* first sub-expression length */
token_type	*p2;		/* second sub-expression pointer */
int		n2;		/* second sub-expression length */
int		*diff_signp;	/* different sign flag pointer */
{
	int	l1, l2;
	int	rv;
#if	DEBUG
	int	rv_should_be_false = false;

	if (n1 < 1 || n2 < 1 || (n1 & 1) != 1 || (n2 & 1) != 1 || diff_signp == NULL || p1 == NULL || p2 == NULL) {
		error_bug("Programming error in call to se_compare().");
	}
#endif
	if (((n1 > n2) ? ((n1 + 1) / (n2 + 1)) : ((n2 + 1) / (n1 + 1))) > 3) {
		/* expressions are grossly different in size, no need to compare, they are different */
#if	DEBUG
		rv_should_be_false = true;
#else
		*diff_signp = false;
		return false;
#endif
	}
	/* Find the proper ground levels of parentheses for the two sub-expressions: */
	l1 = min_level(p1, n1);
	l2 = min_level(p2, n2);
	rv = compare_recurse(p1, n1, l1, p2, n2, l2, diff_signp);
#if	DEBUG
	if (rv && rv_should_be_false) {
		error_bug("Expression compare optimization failed in se_compare().");
	}
#endif
	return rv;
}

/*
 * Recursively compare each parenthesized sub-expression.
 * This is the most used function in Mathomatic.
 * Optimizing or streamlining this would greatly speed things up.
 */
static int
compare_recurse(p1, n1, l1, p2, n2, l2, diff_signp)
token_type	*p1;
int		n1, l1;
token_type	*p2;
int		n2, l2, *diff_signp;
{
#define	compare_epsilon	epsilon

	token_type	*pv1, *ep1, *ep2;
	int		i, j;
	int		len;
	int		first;
	int		oc2;				/* operand count 2 */
	token_type	*opa2[MAX_COMPARE_TERMS];	/* operand pointer array 2 */
	char		used[MAX_COMPARE_TERMS];	/* operand used flag array 2 */
	int		last_op1, op1 = 0, op2 = 0;
	int		diff_op = false;
	double		d1, c1, c2;

	*diff_signp = false;
	if (n1 == 1 && n2 == 1) {
		if (p1->kind != p2->kind) {
			return false;
		}
		switch (p1->kind) {
		case VARIABLE:
			if (sign_cmp_flag && (p1->token.variable & VAR_MASK) == SIGN) {
				return((p2->token.variable & VAR_MASK) == SIGN);
			} else {
				return(p1->token.variable == p2->token.variable);
			}
			break;
		case CONSTANT:
			c1 = p1->token.constant;
			c2 = p2->token.constant;
			if (c1 == c2) {
				return true;
			} else if (c1 == -c2) {
				*diff_signp = true;
				return true;
			}
			d1 = fabs(c1) * compare_epsilon;
			if (fabs(c1 - c2) < d1) {
				return true;
			}
			if (fabs(c1 + c2) < d1) {
				*diff_signp = true;
				return true;
			}
			break;
		case OPERATOR:
			error_bug("Programming error in call to compare_recurse().");
			break;
		}
		return false;
	}
#if	DEBUG
	if (n1 < 1 || n2 < 1 || (n1 & 1) != 1 || (n2 & 1) != 1) {
		error_bug("Programming error in call to compare_recurse().");
	}
#endif
	ep1 = &p1[n1];
	ep2 = &p2[n2];
	for (pv1 = p1 + 1; pv1 < ep1; pv1 += 2) {
		if (pv1->level == l1) {
			op1 = pv1->token.operatr;
			break;
		}
	}
	for (pv1 = p2 + 1; pv1 < ep2; pv1 += 2) {
		if (pv1->level == l2) {
			op2 = pv1->token.operatr;
			break;
		}
	}
	if (op2 == 0) {
		if (op1 != TIMES && op1 != DIVIDE)
			return false;
		goto no_op2;
	}
	switch (op1) {
	case PLUS:
	case MINUS:
		if (op2 != PLUS && op2 != MINUS)
			diff_op = true;
		break;
	case 0:
		if (op2 != TIMES && op2 != DIVIDE)
			return false;
		break;
	case TIMES:
	case DIVIDE:
		if (op2 != TIMES && op2 != DIVIDE)
			diff_op = true;
		break;
	default:
		if (op2 != op1)
			diff_op = true;
		break;
	}
	if (diff_op) {
		if (p1->kind == CONSTANT && p1->level == l1 && op1 == TIMES) {
			if (fabs(fabs(p1->token.constant) - 1.0) <= compare_epsilon) {
				if (!compare_recurse(p1 + 2, n1 - 2, min_level(p1 + 2, n1 - 2), p2, n2, l2, diff_signp)) {
					return false;
				}
				if (p1->token.constant < 0.0) {
					*diff_signp ^= true;
				}
				return true;
			}
		}
		if (p2->kind == CONSTANT && p2->level == l2 && op2 == TIMES) {
			if (fabs(fabs(p2->token.constant) - 1.0) <= compare_epsilon) {
				if (!compare_recurse(p1, n1, l1, p2 + 2, n2 - 2, min_level(p2 + 2, n2 - 2), diff_signp)) {
					return false;
				}
				if (p2->token.constant < 0.0) {
					*diff_signp ^= true;
				}
				return true;
			}
		}
		return false;
	}
no_op2:
	opa2[0] = p2;
	used[0] = false;
	oc2 = 1;
	for (pv1 = p2 + 1; pv1 < ep2; pv1 += 2) {
		if (pv1->level == l2) {
			opa2[oc2] = pv1 + 1;
			used[oc2] = false;
			if (++oc2 >= ARR_CNT(opa2)) {
				debug_string(1, "Expression too big to compare, because MAX_COMPARE_TERMS exceeded.");
				return false;
			}
		}
	}
	opa2[oc2] = pv1 + 1;
	last_op1 = 0;
	first = true;
	for (pv1 = p1;;) {
		for (len = 1; &pv1[len] < ep1; len += 2)
			if (pv1[len].level <= l1)
				break;
		for (i = 0;; i++) {
			if (i >= oc2) {
				if ((op1 == TIMES || op1 == DIVIDE) && pv1->level == l1 && pv1->kind == CONSTANT) {
					if (fabs(fabs(pv1->token.constant) - 1.0) <= compare_epsilon) {
						if (pv1->token.constant < 0.0) {
							*diff_signp ^= true;
						}
						break;
					}
				}
				return false;
			}
			if (used[i]) {
				continue;
			}
			switch (op1) {
			case PLUS:
			case MINUS:
				break;
			case 0:
			case TIMES:
			case DIVIDE:
				if ((last_op1 == DIVIDE) != ((i != 0)
				    && ((opa2[i] - 1)->token.operatr == DIVIDE)))
					continue;
				break;
			default:
				if ((last_op1 == 0) != (i == 0))
					return false;
				break;
			}
			if (compare_recurse(pv1, len, (pv1->level <= l1) ? l1 : (l1 + 1),
			    opa2[i], (opa2[i+1] - opa2[i]) - 1, (opa2[i]->level <= l2) ? l2 : (l2 + 1), &j)) {
				switch (op1) {
				case 0:
				case TIMES:
				case DIVIDE:
					*diff_signp ^= j;
					break;
				case PLUS:
				case MINUS:
					if (last_op1 == MINUS)
						j = !j;
					if (i != 0 && (opa2[i] - 1)->token.operatr == MINUS)
						j = !j;
					if (!first) {
						if (*diff_signp != j)
							continue;
					} else {
						*diff_signp = j;
						first = false;
					}
					break;
/*				case POWER:
Someday fix so that (x-y)^6 compares identical to (y-x)^6.
Also (x-y)^5 should compare identical with different sign to (y-x)^5.
*/
				default:
					if (j)
						continue;
					break;
				}
				used[i] = true;
				break;
			}
		}
		pv1 += len;
		if (pv1 >= ep1)
			break;
		last_op1 = pv1->token.operatr;
		pv1++;
	}
	for (i = 0; i < oc2; i++) {
		if (!used[i]) {
			if ((op2 == TIMES || op2 == DIVIDE) && opa2[i]->level == l2 && opa2[i]->kind == CONSTANT) {
				if (fabs(fabs(opa2[i]->token.constant) - 1.0) <= compare_epsilon) {
					if (opa2[i]->token.constant < 0.0) {
						*diff_signp ^= true;
					}
					continue;
				}
			}
			return false;
		}
	}
	return true;
}

/*
 * Take out meaningless "sign" variables and negative constants.
 * Simplify imaginary numbers raised to the power of some constants.
 *
 * Return true if equation side was modified.
 */
int
elim_sign(equation, np)
token_type	*equation;
int		*np;
{
	int		i, j, k;
	int		level;
	int		modified = false;
	int		op;
	double		d;
	double		numerator, denominator;

	for (i = 1; i < *np; i += 2) {
#if	DEBUG
		if (equation[i].kind != OPERATOR) {
			error_bug("Error in elim_sign().");
		}
#endif
		level = equation[i].level;
		if (equation[i+1].kind == CONSTANT
		    && equation[i].token.operatr == POWER
		    && ((equation[i+1].level == level)
		    || (equation[i+1].level == level + 1))) {
			if (equation[i+1].level == level + 1) {
				if (i + 3 >= *np || equation[i+2].token.operatr != TIMES)
					continue;
				for (k = i + 2; k < *np && equation[k].level >= (level + 1); k += 2)
					;
				if (k <= i + 2)
					continue;
				if (!is_integer_expr(&equation[i+3], k - (i + 3)))
					continue;
			}
			f_to_fraction(equation[i+1].token.constant, &numerator, &denominator);
			if (always_positive(numerator)) {
				if (equation[i-1].level == level
				    && equation[i-1].kind == VARIABLE
				    && equation[i-1].token.variable == IMAGINARY) {
					equation[i-1].kind = CONSTANT;
					equation[i-1].token.constant = -1.0;
					equation[i+1].token.constant /= 2.0;
					modified = true;
					continue;
				}
				op = 0;
				for (j = i - 1; (j >= 0) && equation[j].level >= level; j--) {
					if (equation[j].level <= level + 1) {
						if (equation[j].kind == OPERATOR) {
							op = equation[j].token.operatr;
							break;
						}
					}
				}
				switch (op) {
				case 0:
				case TIMES:
				case DIVIDE:
					for (j = i - 1; (j >= 0) && equation[j].level >= level; j--) {
						if (equation[j].level <= level + 1) {
							if (equation[j].kind == VARIABLE
							    && (equation[j].token.variable & VAR_MASK) == SIGN) {
								equation[j].kind = CONSTANT;
								equation[j].token.constant = 1.0;
								modified = true;
							} else if (equation[j].kind == CONSTANT
							    && equation[j].token.constant < 0.0) {
								equation[j].token.constant = -equation[j].token.constant;
								modified = true;
							}
						}
					}
				}
			} else {
				if (equation[i-1].level == level && equation[i-1].kind == VARIABLE) {
					if (equation[i-1].token.variable == IMAGINARY && equation[i+1].level == level) {
						d = fmod(equation[i+1].token.constant, 4.0);
						if (d == 1.0) {
							equation[i].token.operatr = TIMES;
							equation[i+1].token.constant = 1.0;
							modified = true;
						} else if (d == 3.0) {
							equation[i].token.operatr = TIMES;
							equation[i+1].token.constant = -1.0;
							modified = true;
						}
					} else if ((equation[i-1].token.variable & VAR_MASK) == SIGN) {
						if (fmod(denominator, 2.0) == 1.0) {
							/* odd root, make root (denominator) 1 */
							numerator = fmod(numerator, 2.0);
							/* all sign^2 now translate to 1 */
							if (numerator != equation[i+1].token.constant) {
								equation[i+1].token.constant = numerator;
								modified = true;
							}
						}
					}
				}
			}
		}
	}
	return modified;
}

/*
 * Do complex number division and move the imaginary number from the denominator to the numerator.
 * This is done by multiplying the numerator and denominator by
 * the conjugate of the complex number in the denominator.
 * This often complicates expressions, so use with care.
 *
 * Includes simple division (1/i# -> -1*i#) conversions now.
 *
 * Return true if equation side was modified.
 */
int
div_imaginary(equation, np)
token_type	*equation;
int		*np;
{
	int		i, j, k;
	int		n;
	int		level;
	int		ilevel;
	int		op;
	int		iloc, biloc, eiloc;
	int		eloc;
	int		modified = false;

	for (i = 1; i < *np; i += 2) {
#if	DEBUG
		if (equation[i].kind != OPERATOR) {
			error_bug("Error in div_imaginary().");
		}
#endif
		if (equation[i].token.operatr == DIVIDE) {
			level = equation[i].level;
#if	1	/* This necessary code makes complex number calculations give different (maybe wrong) results. */
		/* It converts x/i to x*-1*i.  Might be better to do this only when symb_flag is true. */
			if (equation[i+1].level == level
			    && equation[i+1].kind == VARIABLE
			    && equation[i+1].token.variable == IMAGINARY) {
				if (*np + 2 > n_tokens) {
					error_huge();
				}
				blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
				*np += 2;
				equation[i].level = level;
				equation[i].kind = OPERATOR;
				equation[i].token.operatr = TIMES;
				i++;
				equation[i].level = level;
				equation[i].kind = CONSTANT;
				equation[i].token.constant = -1.0;
				i++;
				equation[i].level = level;
				equation[i].kind = OPERATOR;
				equation[i].token.operatr = TIMES;
				modified = true;
				continue;
			}
#endif
			op = 0;
			iloc = biloc = eiloc = -1;
			k = i;
			for (j = i + 1; j < *np && equation[j].level > level; j++) {
				if (equation[j].kind == OPERATOR && equation[j].level == level + 1) {
					op = equation[j].token.operatr;
					k = j;
					if (iloc >= 0 && eiloc < 0) {
						eiloc = j;
					}
				} else if (equation[j].kind == VARIABLE && equation[j].token.variable == IMAGINARY) {
					if (iloc >= 0) {
						op = 0;
						break;
					}
					iloc = j;
					biloc = k + 1;
				}
			}
			eloc = j;
			if (iloc >= 0 && eiloc < 0) {
				eiloc = j;
			}
			if (iloc < 0 || (op != PLUS && op != MINUS))
				continue;
			ilevel = equation[iloc].level;
			if (ilevel != level + 1) {
				if (ilevel != level + 2) {
					continue;
				}
				if (iloc > biloc && equation[iloc-1].token.operatr != TIMES)
					continue;
				if (iloc + 1 < eiloc) {
					switch (equation[iloc+1].token.operatr) {
					case TIMES:
					case DIVIDE:
						break;
					default:
						continue;
					}
				}
			}
			if ((eloc - (i + 1)) + 5 + (eiloc - biloc) + *np + 2 > n_tokens)
				error_huge();
			n = eloc - (i + 1);
			blt(scratch, &equation[i+1], n * sizeof(token_type));
			scratch[iloc-(i+1)].kind = CONSTANT;
			scratch[iloc-(i+1)].token.constant = 0.0;
			for (j = 0; j < n; j++)
				scratch[j].level += 2;
			scratch[n].level = level + 2;
			scratch[n].kind = OPERATOR;
			scratch[n].token.operatr = POWER;
			n++;
			scratch[n].level = level + 2;
			scratch[n].kind = CONSTANT;
			scratch[n].token.constant = 2.0;
			n++;
			scratch[n].level = level + 1;
			scratch[n].kind = OPERATOR;
			scratch[n].token.operatr = PLUS;
			n++;
			blt(&scratch[n], &equation[biloc], (eiloc - biloc) * sizeof(token_type));
			j = n;
			n += (eiloc - biloc);
			for (k = j; k < n; k++)
				scratch[k].level += 2;
			scratch[n].level = level + 2;
			scratch[n].kind = OPERATOR;
			scratch[n].token.operatr = POWER;
			n++;
			scratch[n].level = level + 2;
			scratch[n].kind = CONSTANT;
			scratch[n].token.constant = 2.0;
			n++;
			scratch[j+(iloc-biloc)].kind = CONSTANT;
			scratch[j+(iloc-biloc)].token.constant = 1.0;
			blt(&equation[iloc+2], &equation[iloc], (*np - iloc) * sizeof(token_type));
			*np += 2;
			ilevel++;
			equation[iloc].level = ilevel;
			equation[iloc].kind = CONSTANT;
			equation[iloc].token.constant = -1.0;
			iloc++;
			equation[iloc].level = ilevel;
			equation[iloc].kind = OPERATOR;
			equation[iloc].token.operatr = TIMES;
			iloc++;
			equation[iloc].level = ilevel;
			blt(&equation[i+1+n], &equation[i], (*np - i) * sizeof(token_type));
			*np += n + 1;
			blt(&equation[i+1], scratch, n * sizeof(token_type));
			i += n + 1;
			equation[i].token.operatr = TIMES;
			modified = true;
		}
	}
	return modified;
}

/*
 * Convert 1/m*(-1*b+y) to (y-b)/m in equation side.
 *
 * Returns true if equation side was modified.
 * "elim_k()" should be called after this if it returns true.
 *
 * This routine also checks the validity of the equation side.
 */
int
reorder(equation, np)
token_type	*equation;
int		*np;
{
	return(order_recurse(equation, np, 0, 1));
}

static void
swap(equation, np, level, i1, i2)
token_type	*equation;
int		*np;
int		level;
int		i1, i2;
{
	int	e1, e2;
	int	n1, n2;

	for (e1 = i1 + 1;; e1 += 2) {
		if (e1 >= *np || equation[e1].level <= level)
			break;
	}
	for (e2 = i2 + 1;; e2 += 2) {
		if (e2 >= *np || equation[e2].level <= level)
			break;
	}
	n1 = e1 - i1;
	n2 = e2 - i2;
	blt(scratch, &equation[i1], (e2 - i1) * sizeof(token_type));
	if ((i1 + n2) != e1) {
		blt(&equation[i1+n2], &equation[e1], (i2 - e1) * sizeof(token_type));
	}
	blt(&equation[i1], &scratch[i2-i1], n2 * sizeof(token_type));
	blt(&equation[e2-n1], scratch, n1 * sizeof(token_type));
}

/*
 * Recursively check and possibly reorder a "level" sub-expression
 * which starts at "loc" in an equation side.
 */
static int
order_recurse(equation, np, loc, level)
token_type	*equation;
int		*np, loc, level;
{
	int	i, j, k, n;
	int	op = 0;
	int	modified = false;

/* check passed sub-expression for validity: */
	if ((loc & 1) != 0) {
		goto corrupt;
	}
	for (i = loc; i < *np;) {
		if (equation[i].level < level) {
			if (equation[i].kind != OPERATOR || equation[i].level < 1) {
				goto corrupt;
			}
			break;
		}
		if (equation[i].level > level) {
			modified |= order_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i++)
				;
			continue;
		} else {
			if (equation[i].kind == OPERATOR) {
				if ((i & 1) == 0 || equation[i].token.operatr == 0) {
					goto corrupt;
				}
				if (op) {
					switch (equation[i].token.operatr) {
					case PLUS:
					case MINUS:
						if (op == PLUS || op == MINUS)
							break;
						goto corrupt;
					case TIMES:
					case DIVIDE:
						if (op == TIMES || op == DIVIDE)
							break;
					default:
						goto corrupt;
					}
				} else {
					op = equation[i].token.operatr;
				}
			} else {
				if ((i & 1) != 0)
					goto corrupt;
			}
		}
		i++;
	}
	if ((i & 1) == 0) {	/* "i" should be at the end of the current sub-expression */
		goto corrupt;
	}
/* reorder sub-expression if needed: */
	switch (op) {
	case PLUS:
	case MINUS:
		if (equation[loc].kind == CONSTANT && equation[loc].token.constant < 0.0) {
			if (equation[loc].level == level || (equation[loc+1].level == level + 1
			    && (equation[loc+1].token.operatr == TIMES || equation[loc+1].token.operatr == DIVIDE))) {
				for (j = loc + 1; j < i; j += 2) {
					if (equation[j].level == level && equation[j].token.operatr == PLUS) {
						swap(equation, np, level, loc, j + 1);
						modified = true;
						break;
					}
				}
			}
		}
		break;
	case TIMES:
	case DIVIDE:
		for (j = loc + 1;; j += 2) {
			if (j >= i)
				return modified;
			if (equation[j].level == level && equation[j].token.operatr == DIVIDE)
				break;
		}
		for (k = j + 2; k < i;) {
			if (equation[k].level == level && equation[k].token.operatr == TIMES) {
				for (n = k + 2; n < i && equation[n].level > level; n += 2)
					;
				n -= k;
				blt(scratch, &equation[k], n * sizeof(token_type));
				blt(&equation[j+n], &equation[j], (k - j) * sizeof(token_type));
				blt(&equation[j], scratch, n * sizeof(token_type));
				j += n;
				k += n;
				modified = true;
				continue;
			}
			k += 2;
		}
		break;
	}
	return modified;
corrupt:
	error_bug("Internal representation of expression is corrupt!");
	return modified;	/* not reached */
}

/*
 * Try to rationalize the denominator of algebraic fractions.
 * Only works with a square root in the denominator
 * and sometimes helps with simplification.
 *
 * Returns true if something was done.
 * Unfactoring needs to be done immediately, if this returns true.
 */
int
rationalize(equation, np)
token_type	*equation;
int		*np;
{
	int	i, j, k;
	int	i1, k1;
	int	div_level;
	int	end_loc, neg_one_loc = -1;
	int	flag;
	int	modified = false;
	int	count;

	for (i = 1;; i += 2) {
be_thorough:
		if (i >= *np)
			break;
#if	DEBUG
		if (equation[i].kind != OPERATOR) {
			error_bug("Bug in rationalize().");
		}
#endif
		if (equation[i].token.operatr == DIVIDE) {
			div_level = equation[i].level;
			count = 0;
			j = -1;
			for (end_loc = i + 2; end_loc < *np && equation[end_loc].level > div_level; end_loc += 2) {
				if (equation[end_loc].level == (div_level + 1)) {
					count++;
					if (j < 0) {
						j = end_loc;
					}
				}
			}
			if (j < 0)
				continue;
			switch (equation[j].token.operatr) {
			case PLUS:
			case MINUS:
				break;
			default:
				continue;
			}
			i1 = i;
do_again:
			flag = false;
			for (k = j - 2; k > i1; k -= 2) {
				if (equation[k].level == (div_level + 2)) {
					switch (equation[k].token.operatr) {
					case TIMES:
					case DIVIDE:
						flag = 1;
						break;
					case POWER:
						flag = 2;
						break;
					}
					break;
				}
			}
			if (flag) {
				for (k = j - 2; k > i1; k -= 2) {
					if ((equation[k].level == (div_level + 2)
					    || (equation[k].level == (div_level + 3) && flag == 1))
					    && equation[k].token.operatr == POWER
					    && equation[k].level == equation[k+1].level
					    && equation[k+1].kind == CONSTANT
					    && fmod(equation[k+1].token.constant, 1.0) == 0.5) {
						for (k1 = i + 2; k1 < end_loc; k1 += 2) {
							if (equation[k1].token.operatr == POWER
							    && equation[k1].level == equation[k1+1].level
							    && equation[k1+1].kind == CONSTANT
							    && fmod(equation[k1+1].token.constant, 1.0) == 0.5) {
								/* make sure we will actually be eliminating ^.5 from the denominator: */
								if (k != k1 && !(equation[k1].level == (div_level + 2) && count == 1)) {
									i += 2;
									goto be_thorough;
								}
								/* avoid rationalizing absolute values: */
								if (equation[k1-1].level == (equation[k1].level + 1)
								    && equation[k1-2].level == equation[k1-1].level
								    && equation[k1-1].kind == CONSTANT
								    && equation[k1-2].token.operatr == POWER) {
									i += 2;
									goto be_thorough;
								}
							}
						}
						neg_one_loc = i1 + 1;
						k = i1 - i;
						blt(scratch, &equation[i+1], k * sizeof(token_type));
						scratch[k].level = div_level + 2;
						scratch[k].kind = CONSTANT;
						scratch[k].token.constant = -1.0;
						k++;
						scratch[k].level = div_level + 2;
						scratch[k].kind = OPERATOR;
						scratch[k].token.operatr = TIMES;
						k++;
						blt(&scratch[k], &equation[neg_one_loc], (end_loc - neg_one_loc) * sizeof(token_type));
						for (k1 = 0; k1 < (j - neg_one_loc); k1++, k++)
							scratch[k].level++;
						k = end_loc - (i + 1) + 2;
						if (*np + 2 * (k + 1) > n_tokens) {
							error_huge();
						}
						blt(&equation[end_loc+(2*(k+1))], &equation[end_loc], (*np - end_loc) * sizeof(token_type));
						*np += 2 * (k + 1);
						k1 = end_loc;
						equation[k1].level = div_level;
						equation[k1].kind = OPERATOR;
						equation[k1].token.operatr = TIMES;
						k1++;
						blt(&equation[k1], scratch, k * sizeof(token_type));
						k1 += k;
						equation[k1].level = div_level;
						equation[k1].kind = OPERATOR;
						equation[k1].token.operatr = DIVIDE;
						k1++;
						blt(&equation[k1], scratch, k * sizeof(token_type));
						k1 += k;
						debug_string(1, "Square roots in denominator rationalized:");
						side_debug(1, &equation[i+1], k1 - (i + 1));
						i = k1;
						modified = true;
						goto be_thorough;
					}
				}
			}
			if (j >= end_loc)
				continue;
			i1 = j;
			for (j += 2; j < end_loc; j += 2) {
				if (equation[j].level == (div_level + 1)) {
					break;
				}
			}
			goto do_again;
		}
	}
	return modified;
}
