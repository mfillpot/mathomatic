/*
 * Group and combine algebraic fractions for Mathomatic.
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

static int sf_recurse(token_type *equation, int *np, int loc, int level, int start_flag);
static int sf_sub(token_type *equation, int *np, int loc, int i1, int n1, int i2, int n2, int level, int start_flag);

static void
group_recurse(equation, np, loc, level)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
int		loc;		/* starting location within equation side */
int		level;		/* current level of parentheses within the sub-expression starting at "loc" */
{
	int		i;
	int		len;
	int		di = -1, edi;
	int		group_flag = false;
	int		e1;

	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			group_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	edi = e1 = i;
	for (i = loc + 1; i < e1; i += 2) {
		if (equation[i].level == level) {
			if (equation[i].token.operatr == DIVIDE) {
				if (di < 0) {
					di = i;
					continue;
				}
				group_flag = true;
				for (len = i + 2; len < e1; len += 2) {
					if (equation[len].level == level && equation[len].token.operatr != DIVIDE)
						break;
				}
				len -= i;
				if (edi == e1) {
					i += len;
					edi = i;
					continue;
				}
				blt(scratch, &equation[i], len * sizeof(token_type));
				blt(&equation[di+len], &equation[di], (i - di) * sizeof(token_type));
				blt(&equation[di], scratch, len * sizeof(token_type));
				edi += len;
				i += len - 2;
			} else {
				if (di >= 0 && edi == e1)
					edi = i;
			}
		}
	}
	if (group_flag) {
		for (i = di + 1; i < edi; i++) {
			if (equation[i].level == level && equation[i].kind == OPERATOR) {
#if	DEBUG
				if (equation[i].token.operatr != DIVIDE) {
					error_bug("Bug in group_recurse().");
				}
#endif
				equation[i].token.operatr = TIMES;
			}
			equation[i].level++;
		}
	}
}

/*
 * Group denominators of algebraic fractions together in an equation side.
 * Grouping here means converting "a/b/c/d*e" to "a*e/(b*c*d)" or "a/(b*c*d)*e".
 * Not guaranteed to put the grouped divisors last, reorder() puts divisors last.
 */
void
group_proc(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
	group_recurse(equation, np, 0, 1);
}

/*
 * Make equation side ready for display.
 * Basically simplify, then convert non-integer constants in an equation side to fractions,
 * when exactly equal to simple fractions.
 * Also groups algebraic fraction denominators with group_proc() above.
 *
 * Return true if any fractions were created.
 */
int
fractions_and_group(equation, np)
token_type	*equation;
int		*np;
{
	int	rv = false;

	elim_loop(equation, np);
	if (fractions_display) {
		rv = make_fractions(equation, np);
	}
	group_proc(equation, np);
	return rv;
}

/*
 * This function is the guts of the display command.
 * Makes an equation space ready for display.
 *
 * Return true if any fractions were created.
 */
int
make_fractions_and_group(n)
int	n;	/* equation space number */
{
	int	rv = false;

	if (empty_equation_space(n))
		return false;
	if (fractions_and_group(lhs[n], &n_lhs[n]))
		rv = true;
	if (n_rhs[n] > 0) {
		if (fractions_and_group(rhs[n], &n_rhs[n]))
			rv = true;
	}
	return rv;
}

/*
 * Efficiently combine algebraic fractions added together
 * by putting all terms over a common denominator.
 * This means converting "(a/b)+(c/d)+f" directly to "(a*d+c*b+b*d*f)/b/d".
 * The resulting expression is always equivalent to the original expression.
 *
 * If start_flag is 0, only combine denominators to convert complex fractions to simple fractions.
 * Level one addition of fractions will be unchanged.
 * Can make an expression over-complicated.
 *
 * If start_flag is 1, always combine denominators no matter what they are.
 * This can easily make an expression large and complicated,
 * but always results in a single, simple fraction.
 * Used when solving for zero.
 *
 * If start_flag is 2, combine denominators
 * and reduce the result by removing any polynomial GCD between them.
 * Note that this wipes out globals tlhs[] and trhs[].
 * This usually results in the simplest and most correct result.
 *
 * If start_flag is 3: same as start_flag = 2,
 * except absolute value and imaginary denominators are combined, too.
 * This simplifies all algebraic fractions into a single simple fraction.
 * Removing the polynomial GCD usually prevents making a more complicated
 * (or larger) algebraic fraction.
 * Used by the fraction command.
 *
 * Return true if the equation side was modified.
 */
int
super_factor(equation, np, start_flag)
token_type	*equation;	/* pointer to the beginning of the equation side to process */
int		*np;		/* pointer to the length of the equation side */
int		start_flag;
{
	int	rv;

	group_proc(equation, np);
	rv = sf_recurse(equation, np, 0, 1, start_flag);
	organize(equation, np);
	return rv;
}

static int
sf_recurse(equation, np, loc, level, start_flag)
token_type	*equation;
int		*np, loc, level, start_flag;
{
	int	modified = false;
	int	i, j, k;
	int	op;
	int	len1, len2;

	if (!start_flag) {
		for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
			if (equation[i].level == level && equation[i].token.operatr == DIVIDE) {
				start_flag = true;
				break;
			}
		}
	}
	op = 0;
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= sf_recurse(equation, np, i, level + 1, start_flag);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		} else if (equation[i].kind == OPERATOR) {
			op = equation[i].token.operatr;
		}
		i++;
	}
	if (modified || !start_flag)
		return modified;
	switch (op) {
	case PLUS:
	case MINUS:
		break;
	default:
		return modified;
	}
sf_again:
	i = loc;
	for (k = i + 1; k < *np && equation[k].level > level; k += 2)
		;
	len1 = k - i;
	for (j = i + len1 + 1; j < *np && equation[j-1].level >= level; j += len2 + 1) {
		for (k = j + 1; k < *np && equation[k].level > level; k += 2)
			;
		len2 = k - j;
#if	0
		side_debug(0, &equation[i], len1);
		side_debug(0, &equation[j], len2);
#endif
		if (sf_sub(equation, np, loc, i, len1, j, len2, level + 1, start_flag)) {
#if	0
			int junk;
			printf("start_flag = %d\n", start_flag);
			debug_string(0, "super_factor() successful:");
			for (junk = 1; (loc + junk) < *np && equation[loc+junk].level > level; junk += 2)
				;
			side_debug(0, &equation[loc], junk);
#endif
			modified = true;
			goto sf_again;
		}
	}
	return modified;
}

static int
sf_sub(equation, np, loc, i1, n1, i2, n2, level, start_flag)
token_type	*equation;
int		*np, loc, i1, n1, i2, n2, level, start_flag;
{
	int		i, j, k;
	int		b1, b2;
	int		len;
	int		e1, e2;
	int		op1, op2;
	token_type	*p1, *p2;
	int		np1, np2;
	int		div_flag1 = false, div_flag2 = false;
	int		rv;

	e1 = i1 + n1;
	e2 = i2 + n2;
	op2 = equation[i2-1].token.operatr;
	if (i1 <= loc) {
		op1 = PLUS;
	} else {
		op1 = equation[i1-1].token.operatr;
	}
	for (i = i1 + 1; i < e1; i += 2) {
		if (equation[i].level == level && equation[i].token.operatr == DIVIDE) {
			div_flag1 = true;
			break;
		}
	}
	b1 = i + 1;
	if (div_flag1) {
		for (i += 2; i < e1; i += 2) {
			if (equation[i].level <= level)
				break;
		}
	}
	for (j = i2 + 1; j < e2; j += 2) {
		if (equation[j].level == level && equation[j].token.operatr == DIVIDE) {
			div_flag2 = true;
			break;
		}
	}
	b2 = j + 1;
	if (div_flag2) {
		for (j += 2; j < e2; j += 2) {
			if (equation[j].level <= level)
				break;
		}
	}
	if (!div_flag1 && !div_flag2)
		return false;
#if	1
	if (start_flag <= 2 && start_flag != 1) {
#if	0	/* Leave as 0; 1 will not simplify many imaginary number expressions, for example the tangent expansion. */
		if (div_flag1 && found_var(&equation[b1], i - b1, IMAGINARY)) {
			return false;
		}
		if (div_flag2 && found_var(&equation[b2], j - b2, IMAGINARY)) {
			return false;
		}
#endif
		if (div_flag1) {
			if (exp_is_absolute(&equation[b1], i - b1))
				return false;
		}
		if (div_flag2) {
			if (exp_is_absolute(&equation[b2], j - b2))
				return false;
		}
	}
#endif
	if (start_flag >= 2 && div_flag1 && div_flag2) {
#if	DEBUG
		debug_string(1, "Trying to find a polynomial GCD between 2 denominators in sf_sub():");
		side_debug(1, &equation[b1], i - b1);
		side_debug(1, &equation[b2], j - b2);
#endif
		if ((rv = poly2_gcd(&equation[b1], i - b1, &equation[b2], j - b2, 0L, true)) > 0) {
			p1 = tlhs;
			np1 = n_tlhs;
			p2 = trhs;
			np2 = n_trhs;
			goto do_gcd_super;
		}
		if (rv == 0 && poly2_gcd(&equation[b2], j - b2, &equation[b1], i - b1, 0L, true) > 0) {
			p1 = trhs;
			np1 = n_trhs;
			p2 = tlhs;
			np2 = n_tlhs;
			goto do_gcd_super;
		}
#if	DEBUG
		debug_string(1, "Done; polynomial GCD not found.");
#endif
	}
        if (n1 + n2 + (i - b1) + (j - b2) + 8 > n_tokens) {
                error_huge();
        } 
	if (!div_flag1) {
		for (k = i1; k < e1; k++)
			equation[k].level++;
	}
	if (!div_flag2) {
		for (k = i2; k < e2; k++)
			equation[k].level++;
	}
	len = (b1 - i1) - 1;
	blt(scratch, &equation[i1], len * sizeof(token_type));
	if (op1 == MINUS) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		scratch[len].level = level;
		scratch[len].kind = CONSTANT;
		scratch[len].token.constant = -1.0;
		len++;
	}
	if (div_flag1) {
		blt(&scratch[len], &equation[i], (e1 - i) * sizeof(token_type));
		len += e1 - i;
	}
	if (div_flag2) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		blt(&scratch[len], &equation[b2], (j - b2) * sizeof(token_type));
		len += j - b2;
	}
	for (k = 0; k < len; k++)
		scratch[k].level += 2;
	scratch[len].level = level + 1;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = op2;
	len++;
	k = len;
	blt(&scratch[len], &equation[i2], (b2 - i2 - 1) * sizeof(token_type));
	len += b2 - i2 - 1;
	if (div_flag2) {
		blt(&scratch[len], &equation[j], (e2 - j) * sizeof(token_type));
		len += e2 - j;
	}
	if (div_flag1) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
		len += i - b1;
	}
	for (; k < len; k++)
		scratch[k].level += 2;
	scratch[len].level = level;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = DIVIDE;
	len++;
	k = len;
	if (div_flag1) {
		blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
		len += i - b1;
	}
	if (div_flag1 && div_flag2) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
	}
	if (div_flag2) {
		blt(&scratch[len], &equation[b2], (j - b2) * sizeof(token_type));
		len += j - b2;
	}
	for (; k < len; k++)
		scratch[k].level++;
end_mess:
	if (*np + len - n1 - (n2 + 1) > n_tokens) {
		error_huge();
	}
	if (op1 == MINUS) {
		equation[i1-1].token.operatr = PLUS;
	}
	blt(&equation[i2-1], &equation[e2], (*np - e2) * sizeof(token_type));
	*np -= n2 + 1;
	blt(&equation[i1+len], &equation[e1], (*np - e1) * sizeof(token_type));
	*np += len - n1;
	blt(&equation[i1], scratch, len * sizeof(token_type));
	return true;

do_gcd_super:
#if	DEBUG
	debug_string(1, "Found a polynomial GCD between 2 denominators in sf_sub().");
#endif

	if (5 - i1 + e1 + (2*np2) + b2 - i2 + e2 - j + np1 > n_tokens) {
		error_huge();
	}
	for (k = 0; k < np1; k++) {
		p1[k].level += level;
	}
	for (k = 0; k < np2; k++) {
		p2[k].level += level;
	}
	len = (b1 - i1) - 1;
	blt(scratch, &equation[i1], len * sizeof(token_type));
	if (op1 == MINUS) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		scratch[len].level = level;
		scratch[len].kind = CONSTANT;
		scratch[len].token.constant = -1.0;
		len++;
	}
/*	if (div_flag1) { */
		blt(&scratch[len], &equation[i], (e1 - i) * sizeof(token_type));
		len += e1 - i;
/*	} */
/*	if (div_flag2) { */
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		blt(&scratch[len], p2, np2 * sizeof(token_type));
		len += np2;
/*	} */
	for (k = 0; k < len; k++)
		scratch[k].level += 2;
	scratch[len].level = level + 1;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = op2;
	len++;
	k = len;
	blt(&scratch[len], &equation[i2], (b2 - i2 - 1) * sizeof(token_type));
	len += b2 - i2 - 1;
/*	if (div_flag2) { */
		blt(&scratch[len], &equation[j], (e2 - j) * sizeof(token_type));
		len += e2 - j;
/*	} */
/*	if (div_flag1) { */
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		blt(&scratch[len], p1, np1 * sizeof(token_type));
		len += np1;
/*	} */
	for (; k < len; k++)
		scratch[k].level += 2;
	scratch[len].level = level;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = DIVIDE;
	len++;
	k = len;
/*	if (div_flag1) { */
		blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
		len += (i - b1);
/*	} */
/*	if (div_flag1 && div_flag2) { */
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
/*	} */
/*	if (div_flag2) { */
		blt(&scratch[len], p2, np2 * sizeof(token_type));
		len += np2;
/*	} */
	for (; k < len; k++)
		scratch[k].level++;
	goto end_mess;
}
