/*
 * Mathomatic symbolic factorizing routines, not polynomial factoring.
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

#define	ALWAYS_FACTOR_POWER	1

static int fplus_recurse(token_type *equation, int *np, int loc, int level, long v, double d, int whole_flag, int div_only);
static int fplus_sub(token_type *equation, int *np, int loc, int i1, int n1, int i2, int n2, int level, long v, double d, int whole_flag, int div_only);
static int big_fplus(token_type *equation, int level, int diff_sign, int sop1, int op1, int op2, int i1, int i2, int b1, int b2, int ai, int aj, int i, int j, int e1, int e2);
static int ftimes_recurse(token_type *equation, int *np, int loc, int level);
static int ftimes_sub(token_type *equation, int *np, int loc, int i1, int n1, int i2, int n2, int level);
static int fpower_recurse(token_type *equation, int *np, int loc, int level);
static int fpower_sub(token_type *equation, int *np, int loc, int i1, int n1, int i2, int n2, int level);

/*
 * Factor divides only: (a/c + b/c) -> ((a+b)/c).
 *
 * Return true if equation side was modified.
 */
int
factor_divide(equation, np, v, d)
token_type	*equation;
int		*np;
long		v;
double		d;
{
	return fplus_recurse(equation, np, 0, 1, v, d, false, true);
}

/*
 * Take care of subtraction and addition of the same expression
 * multiplied by constants: (2*a + 3*a - a) -> (4*a).
 *
 * Return true if equation side was modified.
 */
int
subtract_itself(equation, np)
token_type	*equation;
int		*np;
{
	return fplus_recurse(equation, np, 0, 1, 0L, 0.0, true, false);
}

/*
 * Factor equation side: (a*c + b*c) -> (c*(a + b)).
 *
 * If "v" and "d" equal 0, factor anything,
 * including identical bases raised to powers (Horner factoring): (x^2 + x) -> (x*(x + 1)).
 * If "d" equals 1.0, only factor identical bases raised to the power of a constant.
 *
 * If "v" is a variable, or MATCH_ANY, only factor expressions containing that variable,
 * or any variable, respectively, with no Horner factoring.
 * IF "v" is not MATCH_ANY, and "d" is not equal to 0.0 or 1.0,
 * factor only expressions containing "v" raised to the power of "d".
 *
 * In order to factor down to the smallest possible expression,
 * factoring the most common and largest sub-expressions needs to be done first.
 * Mathomatic does not have the ability to do that.
 * However, simpb_side() factors expressions with the most used variables first,
 * which is helpful.
 *
 * Return true if equation side was modified.
 */
int
factor_plus(equation, np, v, d)
token_type	*equation;	/* pointer to beginning of equation side */
int		*np;		/* pointer to length of equation side */
long		v;		/* Mathomatic variable */
double		d;		/* control exponent */
{
	return fplus_recurse(equation, np, 0, 1, v, d, false, false);
}

/*
 * Recursively factor at "level" of parentheses and deeper,
 * beginning at index "loc".
 *
 * Return true if equation side was modified.
 */
static int
fplus_recurse(equation, np, loc, level, v, d, whole_flag, div_only)
token_type	*equation;
int		*np, loc, level;
long		v;
double		d;
int		whole_flag;	/* factor only whole expressions multiplied by a constant */
int		div_only;	/* factor only divides */
{
	int	modified = false;
	int	i, j, k;
	int	op = 0;
	int	len1, len2;

	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		if (equation[i].level == level) {
			op = equation[i].token.operatr;
			break;
		}
	}
	switch (op) {
	case PLUS:
	case MINUS:
		for (i = loc;;) {
f_again:
			for (k = i + 1;; k += 2) {
				if (k >= *np || equation[k].level <= level)
					break;
			}
			len1 = k - i;
			for (j = i + len1 + 1;; j += len2 + 1) {
				if (j >= *np || equation[j-1].level < level)
					break;
				for (k = j + 1;; k += 2) {
					if (k >= *np || equation[k].level <= level)
						break;
				}
				len2 = k - j;
				if (fplus_sub(equation, np, loc, i, len1, j, len2, level + 1, v, d, whole_flag, div_only)) {
					modified = true;
					goto f_again;
				}
			}
			i += len1 + 1;
			if (i >= *np || equation[i-1].level < level)
				break;
		}
	}
	if (modified)
		return true;
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= fplus_recurse(equation, np, i, level + 1, v, d, whole_flag, div_only);
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
 * Do the factoring of two sub-expressions added together.
 *
 * Return true if a transformation was made.
 */
static int
fplus_sub(equation, np, loc, i1, n1, i2, n2, level, v, d, whole_flag, div_only)
token_type	*equation;	/* the entire expression */
int		*np;		/* pointer to length of the entire expression */
int		loc;		/* index into the beginning of this additive sub-expression */
int		i1, n1, i2, n2;
int		level;
long		v;
double		d;
int		whole_flag;	/* factor only whole expressions multiplied by a constant */
int		div_only;	/* factor only divides */
{
	int	e1, e2;
	int	op1, op2;
	int	i, j, k, l, m;
	int	b1, b2;
	int	len;
	int	sop1;
	int	diff_sign;
	int	ai, aj;
	int	flag1, flag2;
	int	same_flag;
	double	save_k1, save_k2;
	double	save_d1, save_d2;
	double	power;		/* for constant power horner factoring */
	double	d1, d2;

	e1 = i1 + n1;
	e2 = i2 + n2;
	op2 = equation[i2-1].token.operatr;
	if (i1 <= loc) {
		op1 = PLUS;
	} else {
		op1 = equation[i1-1].token.operatr;
	}
	i = i1 - 1;
f_outer:
	b1 = i + 1;
	if (b1 >= e1)
		return false;
	if (whole_flag) {
		i = e1;
		if (n1 > 1 && equation[b1].kind == CONSTANT
		    && equation[b1+1].level == level
		    && (equation[b1+1].token.operatr == TIMES
		    || equation[b1+1].token.operatr == DIVIDE)) {
			b1 += 2;
		}
	} else {
		i = b1 + 1;
		for (; i < e1; i += 2) {
			if (equation[i].level == level
			    && (equation[i].token.operatr == TIMES
			    || equation[i].token.operatr == DIVIDE)) {
				break;
			}
		}
	}
	if (b1 <= i1)
		sop1 = TIMES;
	else
		sop1 = equation[b1-1].token.operatr;
	if ((div_only && sop1 != DIVIDE)
	    || (i - b1 == 1 && equation[b1].kind == CONSTANT && fabs(equation[b1].token.constant) == 1.0)) {
		goto f_outer;
	}
	if (!whole_flag && (v != MATCH_ANY)) {
		if (d == 0.0 || d == 1.0) {
			if (v) {
				for (k = b1;; k += 2) {
					if (k >= i)
						goto f_outer;
					if (equation[k].kind == VARIABLE && equation[k].token.variable == v) {
						break;
					}
				}
			}
		} else {
			for (k = b1 + 1;; k += 2) {
				if (k >= i)
					goto f_outer;
				if (equation[k].token.operatr == POWER
				    && equation[k].level == equation[k+1].level
				    && equation[k+1].kind == CONSTANT
				    && equation[k+1].token.constant == d) {
					if (v == 0)
						goto factor_this;
					for (l = k - 1; l >= 0; l--) {
						if (equation[l].level < equation[k].level)
							break;
						if (equation[l].kind == VARIABLE
						    && equation[l].token.variable == v)
							goto factor_this;
					}
				}
			}
		}
	}
factor_this:
	j = i2 - 1;
f_inner:
	b2 = j + 1;
	if (b2 >= e2)
		goto f_outer;
	if (whole_flag) {
		j = e2;
		if (n2 > 1 && equation[b2].kind == CONSTANT
		    && equation[b2+1].level == level
		    && (equation[b2+1].token.operatr == TIMES
		    || equation[b2+1].token.operatr == DIVIDE)) {
			b2 += 2;
		}
	} else {
		j = b2 + 1;
		for (; j < e2; j += 2) {
			if (equation[j].level == level
			    && (equation[j].token.operatr == TIMES
			    || equation[j].token.operatr == DIVIDE)) {
				break;
			}
		}
		if (b2 <= i2) {
			if (sop1 == DIVIDE)
				goto f_inner;
		} else {
			if (equation[b2-1].token.operatr != sop1)
				goto f_inner;
		}
	}
	if (j - b2 == 1 && equation[b2].kind == CONSTANT && fabs(equation[b2].token.constant) == 1.0) {
		goto f_inner;
	}
	ai = i;
	aj = j;
	save_k1 = 0.0;
	save_k2 = 0.0;
	flag1 = (whole_flag && b1 > i1);
	if (flag1) {
		b1 = i1;
		save_k1 = equation[b1].token.constant;
		equation[b1].token.constant = 1.0;
	}
	flag2 = (whole_flag && b2 > i2);
	if (flag2) {
		b2 = i2;
		save_k2 = equation[b2].token.constant;
		equation[b2].token.constant = 1.0;
	}
	same_flag = se_compare(&equation[b1], i - b1, &equation[b2], j - b2, &diff_sign);
	if (flag1) {
		equation[i1].token.constant = save_k1;
		b1 += 2;
	}
	if (flag2) {
		equation[i2].token.constant = save_k2;
		b2 += 2;
	}
	if (same_flag) {
		/* do the factor transformation */
		power = 1.0;	/* not doing Horner factoring */
horner_factor:
		if (sop1 == DIVIDE) {
			scratch[0].level = level;
			scratch[0].kind = CONSTANT;
			scratch[0].token.constant = 1.0;
			scratch[1].level = level;
			scratch[1].kind = OPERATOR;
			scratch[1].token.operatr = DIVIDE;
			len = 2;
		} else {
			len = 0;
		}
		k = len;
		blt(&scratch[len], &equation[b1], (ai - b1) * sizeof(token_type));
		len += ai - b1;
		if (power != 1.0) {
			for (; k < len; k++)
				scratch[k].level += 2;
			scratch[len].level = level + 1;
			scratch[len].kind = OPERATOR;
			scratch[len].token.operatr = POWER;
			len++;
			scratch[len].level = level + 1;
			scratch[len].kind = CONSTANT;
			scratch[len].token.constant = power;
			len++;
			if (always_positive(power))
				diff_sign = false;
		} else if (b1 == i1 && ai == e1) {
			for (; k < len; k++)
				scratch[k].level++;
		}
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		k = len;
		blt(&scratch[len], &equation[i1], (b1 - i1) * sizeof(token_type));
		len += b1 - i1;
		if (ai != i) {
			l = len;
			m = len + ai - b1;
			blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
			len += i - b1;
			if (b1 == i1 && i == e1) {
				for (; l < len; l++)
					scratch[l].level++;
			}
			l = m;
			m++;
			for (; m < len; m++)
				scratch[m].level++;
			scratch[len].level = scratch[l].level + 1;
			scratch[len].kind = OPERATOR;
			scratch[len].token.operatr = MINUS;
			len++;
			scratch[len].level = scratch[l].level + 1;
			scratch[len].kind = CONSTANT;
			scratch[len].token.constant = power;
			len++;
			scratch[len].level = level;
			scratch[len].kind = OPERATOR;
			scratch[len].token.operatr = TIMES;
			len++;
		}
		scratch[len].level = level;
		scratch[len].kind = CONSTANT;
		if (op1 == MINUS) {
			scratch[len].token.constant = -1.0;
		} else {
			scratch[len].token.constant = 1.0;
		}
		len++;
		blt(&scratch[len], &equation[i], (e1 - i) * sizeof(token_type));
		len += e1 - i;
		for (; k < len; k++)
			scratch[k].level += 2;
		scratch[len].level = level + 1;
		scratch[len].kind = OPERATOR;
		diff_sign ^= (op2 == MINUS);
		if (diff_sign) {
			scratch[len].token.operatr = MINUS;
		} else {
			scratch[len].token.operatr = PLUS;
		}
		len++;
		k = len;
		if (aj != j) {
			if (len + n2 + 2 > n_tokens) {
				error_huge();
			}
		} else {
			if (len + (b2 - i2) + (e2 - j) + 1 > n_tokens) {
				error_huge();
			}
		}
		blt(&scratch[len], &equation[i2], (b2 - i2) * sizeof(token_type));
		len += b2 - i2;
		if (aj != j) {
			m = len + aj - b2;
			blt(&scratch[len], &equation[b2], (j - b2) * sizeof(token_type));
			len += j - b2;
			l = m;
			m++;
			for (; m < len; m++)
				scratch[m].level++;
			scratch[len].level = scratch[l].level + 1;
			scratch[len].kind = OPERATOR;
			scratch[len].token.operatr = MINUS;
			len++;
			scratch[len].level = scratch[l].level + 1;
			scratch[len].kind = CONSTANT;
			scratch[len].token.constant = power;
			len++;
		} else {
			scratch[len].level = level;
			scratch[len].kind = CONSTANT;
			scratch[len].token.constant = 1.0;
			len++;
		}
		blt(&scratch[len], &equation[j], (e2 - j) * sizeof(token_type));
		len += e2 - j;
		for (; k < len; k++)
			scratch[k].level += 2;
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
	}
	if (whole_flag)
		return false;
	if (v || div_only)
		goto f_inner;	/* don't do Horner factoring */
	if (b1 == i1 && i == e1)
		k = level;
	else
		k = level + 1;
	save_d1 = 1.0;
	for (l = b1 + 1;; l += 2) {
		if (l >= i) {
			break;
		}
		if (equation[l].level == k && equation[l].token.operatr == POWER) {
			if (equation[l+1].level == k && equation[l+1].kind == CONSTANT) {
				save_d1 = equation[l+1].token.constant;
				if (save_d1 <= 0.0)
					goto f_inner;
			} else {
				save_d1 = -1.0;
			}
			ai = l;
			break;
		}
	}
	if (b2 == i2 && j == e2)
		k = level;
	else
		k = level + 1;
	save_d2 = 1.0;
	for (l = b2 + 1;; l += 2) {
		if (l >= j) {
			break;
		}
		if (equation[l].level == k && equation[l].token.operatr == POWER) {
			if (equation[l+1].level == k && equation[l+1].kind == CONSTANT) {
				save_d2 = equation[l+1].token.constant;
				if (save_d2 <= 0.0)
					goto f_inner;
			} else {
				save_d2 = -1.0;
			}
			aj = l;
			break;
		}
	}
	if (ai == i && aj == j)
		goto f_inner;
	if (ai - b1 == 1 && equation[b1].kind == CONSTANT)
		goto f_inner;
	if (d == 1.0 && (save_d1 < 0.0 || save_d2 < 0.0))
		goto f_inner;
	if (se_compare(&equation[b1], ai - b1, &equation[b2], aj - b2, &diff_sign)) {
		if (save_d1 > 0.0 || save_d2 > 0.0) {
			if (save_d1 < 0.0) {
				power = save_d2;
			} else if (save_d2 < 0.0) {
				power = save_d1;
			} else {
				power = min(save_d1, save_d2);
				if (!diff_sign && (fmod(power, 1.0) != 0.0)) {
					if (fmod(max(save_d1, save_d2) - power, 1.0) == 0.0) {
						goto horner_factor;
					}
				}
			}
			if (power < 1.0)
				goto f_inner;
			modf(power, &power);	/* same as power = trunc(power); ? */
			goto horner_factor;
		}
		d1 = i - ai;
		d2 = j - aj;
		if (d1 == d2) {
			d1 = 1.0;
			d2 = 1.0;
			if ((ai + 2) < i) {
				k = equation[ai].level;
				if (equation[ai+1].level == (k + 1)
				    && equation[ai+2].level == (k + 1)
				    && equation[ai+1].kind == CONSTANT
				    && (equation[ai+2].token.operatr == TIMES
				    || equation[ai+2].token.operatr == DIVIDE)) {
					d1 = fabs(equation[ai+1].token.constant);
				}
			}
			if ((aj + 2) < j) {
				k = equation[aj].level;
				if (equation[aj+1].level == (k + 1)
				    && equation[aj+2].level == (k + 1)
				    && equation[aj+1].kind == CONSTANT
				    && (equation[aj+2].token.operatr == TIMES
				    || equation[aj+2].token.operatr == DIVIDE)) {
					d2 = fabs(equation[aj+1].token.constant);
				}
			}
		}
		if (d1 <= d2) {
			len = big_fplus(equation, level, diff_sign, sop1,
			    op1, op2, i1, i2, b1, b2, ai, aj, i, j, e1, e2);
		} else {
			len = big_fplus(equation, level, diff_sign, sop1,
			    op2, op1, i2, i1, b2, b1, aj, ai, j, i, e2, e1);
		}
		goto end_mess;
	}
	goto f_inner;
}

/*
 * Factor transformation for a more general pair of sub-expressions added together
 * with a common base and any exponent.
 */
static int
big_fplus(equation, level, diff_sign, sop1, op1, op2, i1, i2, b1, b2, ai, aj, i, j, e1, e2)
token_type	*equation;
int		level;
int		diff_sign;
int		sop1;
int		op1, op2;
int		i1, i2;
int		b1, b2;
int		ai, aj;
int		i, j;
int		e1, e2;
{
	int	k, l, m, n, o;
	int	len;

	if (sop1 == DIVIDE) {
		scratch[0].level = level;
		scratch[0].kind = CONSTANT;
		scratch[0].token.constant = 1.0;
		scratch[1].level = level;
		scratch[1].kind = OPERATOR;
		scratch[1].token.operatr = DIVIDE;
		len = 2;
	} else {
		len = 0;
	}
	k = len;
	o = len + ai - b1;
	blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
	len += i - b1;
	if (b1 == i1 && i == e1) {
		for (; k < len; k++)
			scratch[k].level++;
	}
	scratch[len].level = level;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = TIMES;
	len++;
	k = len;
	blt(&scratch[len], &equation[i1], (b1 - i1) * sizeof(token_type));
	len += b1 - i1;
	scratch[len].level = level;
	scratch[len].kind = CONSTANT;
	if (op1 == MINUS) {
		scratch[len].token.constant = -1.0;
	} else {
		scratch[len].token.constant = 1.0;
	}
	len++;
	blt(&scratch[len], &equation[i], (e1 - i) * sizeof(token_type));
	len += e1 - i;
	for (; k < len; k++)
		scratch[k].level += 2;
	scratch[len].level = level + 1;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = op2;
	len++;
	k = len;
	blt(&scratch[len], &equation[i2], (b2 - i2) * sizeof(token_type));
	len += b2 - i2;
	if (len + (e2 - b2) + 2 * (i - ai) + 2 > n_tokens) {
		error_huge();
	}
	n = len;
	m = len + aj - b2;
	blt(&scratch[len], &equation[b2], (j - b2) * sizeof(token_type));
	len += j - b2;
	l = m;
	m++;
	for (; m < len; m++)
		scratch[m].level++;
	if (diff_sign && b2 == i2 && j == e2) {
		for (; n < len; n++)
			scratch[n].level++;
	}
	scratch[len].level = scratch[l].level + 1;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = MINUS;
	len++;
	m = len;
	blt(&scratch[len], &equation[ai+1], (i - (ai + 1)) * sizeof(token_type));
	len += i - (ai + 1);
	n = min_level(&equation[ai+1], i - (ai + 1));
	n = scratch[l].level + 2 - n;
	for (; m < len; m++)
		scratch[m].level += n;
	if (diff_sign) {
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		if (sop1 == DIVIDE)
			scratch[len].token.operatr = TIMES;
		else
			scratch[len].token.operatr = DIVIDE;
		len++;
		scratch[len].level = level + 1;
		scratch[len].kind = CONSTANT;
		scratch[len].token.constant = -1.0;
		len++;
		blt(&scratch[len], &scratch[o], (i - ai) * sizeof(token_type));
		len += i - ai;
	}
	blt(&scratch[len], &equation[j], (e2 - j) * sizeof(token_type));
	len += e2 - j;
	for (; k < len; k++)
		scratch[k].level += 2;
	return len;
}

/*
 * Factor equation side.
 * a^b * a^c -> a^(b + c).
 * Return true if equation side was modified.
 */
int
factor_times(equation, np)
token_type	*equation;
int		*np;
{
	return ftimes_recurse(equation, np, 0, 1);
}

static int
ftimes_recurse(equation, np, loc, level)
token_type	*equation;
int		*np, loc, level;
{
	int	modified = false;
	int	i, j, k;
	int	op = 0;
	int	len1, len2;

	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		if (equation[i].level == level) {
			op = equation[i].token.operatr;
			break;
		}
	}
	switch (op) {
	case TIMES:
	case DIVIDE:
		for (i = loc;;) {
f_again:
			for (k = i + 1;; k += 2) {
				if (k >= *np || equation[k].level <= level)
					break;
			}
			len1 = k - i;
			for (j = i + len1 + 1;; j += len2 + 1) {
				if (j >= *np || equation[j-1].level < level)
					break;
				for (k = j + 1;; k += 2) {
					if (k >= *np || equation[k].level <= level)
						break;
				}
				len2 = k - j;
				if (ftimes_sub(equation, np, loc, i, len1, j, len2, level + 1)) {
					modified = true;
					goto f_again;
				}
			}
			i += len1 + 1;
			if (i >= *np || equation[i-1].level < level)
				break;
		}
	}
	if (modified)
		return true;
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= ftimes_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	return modified;
}

static int
ftimes_sub(equation, np, loc, i1, n1, i2, n2, level)
token_type	*equation;
int		*np, loc, i1, n1, i2, n2, level;
{
	int	e1, e2;
	int	op1, op2;
	int	i, j, k;
	int	len, rlen1, len2;
	int	diff_sign;
	int	both_divide;

	e1 = i1 + n1;
	e2 = i2 + n2;
	op2 = equation[i2-1].token.operatr;
	if (i1 <= loc) {
		op1 = TIMES;
	} else {
		op1 = equation[i1-1].token.operatr;
	}
#if	1
	if ((n1 == 1 && equation[i1].kind == CONSTANT) && (n2 == 1 && equation[i2].kind == CONSTANT)) {
		return false;
	}
#else
	if ((n1 == 1 && equation[i1].kind == CONSTANT) || (n2 == 1 && equation[i2].kind == CONSTANT)) {
		return false;
	}
#endif
	both_divide = (op1 == DIVIDE && op2 == DIVIDE);
	if (se_compare(&equation[i1], n1, &equation[i2], n2, &diff_sign)) {
		i = e1;
		j = e2;
		goto common_base;
	}
	for (i = i1 + 1; i < e1; i += 2) {
		if (equation[i].level == level && equation[i].token.operatr == POWER) {
			break;
		}
	}
	for (j = i2 + 1; j < e2; j += 2) {
		if (equation[j].level == level && equation[j].token.operatr == POWER) {
			break;
		}
	}
	if (i >= e1 && j >= e2) {
		return false;
	}
	if (se_compare(&equation[i1], i - i1, &equation[i2], j - i2, &diff_sign)) {
		goto common_base;
	}
	if (i < e1 && j < e2) {
		if (se_compare(&equation[i1], n1, &equation[i2], j - i2, &diff_sign)) {
			i = e1;
			goto common_base;
		}
		if (se_compare(&equation[i1], i - i1, &equation[i2], n2, &diff_sign)) {
			j = e2;
			goto common_base;
		}
	}
	return false;

common_base:
	rlen1 = ((i == e1) ? 1 : (e1 - i - 1));
	len = (i - i1) + 1 + ((op1 == DIVIDE && !both_divide) ? 2 : 0) + rlen1 + 1
	    + ((j == e2) ? 1 : (e2 - j - 1));
	len -= n1;
	if (j - i2 == 1 && equation[i2].kind == CONSTANT && equation[i2].token.constant == -1.0)
		return false;
	if (diff_sign) {
		if (j - i2 == 1 && equation[i2].kind == CONSTANT)
			return false;
		len2 = 2 + e2 - j;
		if (*np + len2 + len > n_tokens) {
			error_huge();
		}
		blt(&equation[e2+len2], &equation[e2], (*np - e2) * sizeof(token_type));
		*np += len2;
		equation[e2].level = level - 1;
		equation[e2].kind = OPERATOR;
		equation[e2].token.operatr = op2;
		equation[e2+1].level = level;
		equation[e2+1].kind = CONSTANT;
		equation[e2+1].token.constant = -1.0;
		blt(&equation[e2+2], &equation[j], (e2 - j) * sizeof(token_type));
	}
	if (*np + len > n_tokens) {
		error_huge();
	}
	blt(&equation[e1+len], &equation[e1], (*np - e1) * sizeof(token_type));
	*np += len;
	if (i == e1) {
		for (k = i1; k < e1; k++)
			equation[k].level++;
		equation[i].level = level;
		equation[i].kind = OPERATOR;
		equation[i].token.operatr = POWER;
		equation[i+1].level = level;
		equation[i+1].kind = CONSTANT;
		equation[i+1].token.constant = 1.0;
	}
	if (op1 == DIVIDE && !both_divide) {
		equation[i1-1].token.operatr = TIMES;
		blt(&equation[i+3], &equation[i+1], rlen1 * sizeof(token_type));
		i++;
		equation[i].level = level;
		equation[i].kind = CONSTANT;
		equation[i].token.constant = -1.0;
		i++;
		equation[i].level = level;
		equation[i].kind = OPERATOR;
		equation[i].token.operatr = TIMES;
		binary_parenthesize(equation, i + 1 + rlen1, i);
	}
	i += rlen1 + 1;
	equation[i].level = level;
	equation[i].kind = OPERATOR;
	if (op2 == DIVIDE && !both_divide)
		equation[i].token.operatr = MINUS;
	else
		equation[i].token.operatr = PLUS;
	if (j == e2) {
		equation[i+1].level = level;
		equation[i+1].kind = CONSTANT;
		equation[i+1].token.constant = 1.0;
		binary_parenthesize(equation, i + 2, i);
	} else {
		blt(&equation[i+1], &equation[j+len+1], (e2 - j - 1) * sizeof(token_type));
		binary_parenthesize(equation, i + e2 - j, i);
	}
	blt(&equation[i2+len-1], &equation[e2+len], (*np - (e2 + len)) * sizeof(token_type));
	*np -= n2 + 1;
	return true;
}

/*
 * Factor equation side.
 * a^c * b^c -> (a * b)^c.
 * Return true if equation side was modified.
 */
int
factor_power(equation, np)
token_type	*equation;
int		*np;
{
	return fpower_recurse(equation, np, 0, 1);
}

static int
fpower_recurse(equation, np, loc, level)
token_type	*equation;
int		*np, loc, level;
{
	int	modified = false;
	int	i, j, k;
	int	op = 0;
	int	len1, len2;

	for (i = loc + 1; i < *np && equation[i].level >= level; i += 2) {
		if (equation[i].level == level) {
			op = equation[i].token.operatr;
			break;
		}
	}
	switch (op) {
	case TIMES:
	case DIVIDE:
		for (i = loc;;) {
f_again:
			for (k = i + 1;; k += 2) {
				if (k >= *np || equation[k].level <= level)
					break;
			}
			len1 = k - i;
			for (j = i + len1 + 1;; j += len2 + 1) {
				if (j >= *np || equation[j-1].level < level)
					break;
				for (k = j + 1;; k += 2) {
					if (k >= *np || equation[k].level <= level)
						break;
				}
				len2 = k - j;
				if (fpower_sub(equation, np, loc, i, len1, j, len2, level + 1)) {
					modified = true;
					goto f_again;
				}
			}
			i += len1 + 1;
			if (i >= *np || equation[i-1].level < level)
				break;
		}
	}

	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].level > level) {
			modified |= fpower_recurse(equation, np, i, level + 1);
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	return modified;
}

static int
fpower_sub(equation, np, loc, i1, n1, i2, n2, level)
token_type	*equation;
int		*np, loc, i1, n1, i2, n2, level;
{
	int		e1, e2;
	int		op1, op2;
	int		pop1 = TIMES;
	int		start2;
	int		i, j, k;
	int		b1, b2;
	int		len;
	int		diff_sign;
	int		all_divide;
#if	!ALWAYS_FACTOR_POWER
	int		abs1_flag = false, abs2_flag = false;
#endif

	e1 = i1 + n1;
	e2 = i2 + n2;
	op2 = equation[i2-1].token.operatr;
	if (i1 <= loc) {
		op1 = TIMES;
	} else {
		op1 = equation[i1-1].token.operatr;
	}
	for (i = i1 + 1;; i += 2) {
		if (i >= e1)
			return false;
		if (equation[i].level == level && equation[i].token.operatr == POWER) {
#if	!ALWAYS_FACTOR_POWER
/* avoid absolute values: */
			if (i > (i1 + 2)
			    && ((equation[i+1].level == level && (i + 2) == e1)
			    || (equation[i+1].level == (level + 1)
			    && (i + 4) == e1
			    && equation[i+2].level == (level + 1)
			    && equation[i+2].token.operatr == DIVIDE
			    && equation[i+3].level == (level + 1)
			    && equation[i+3].kind == CONSTANT))
			    && equation[i-1].level == (level + 1)
			    && equation[i+1].kind == CONSTANT
			    && equation[i-1].kind == CONSTANT
			    && equation[i-2].level == (level + 1) && equation[i-2].token.operatr == POWER) {
				abs1_flag = true;
/*				return false;	*/
			}
#endif
			break;
		}
	}
	for (j = i2 + 1;; j += 2) {
		if (j >= e2)
			return false;
		if (equation[j].level == level && equation[j].token.operatr == POWER) {
#if	!ALWAYS_FACTOR_POWER
/* avoid absolute values: */
			if (j > (i2 + 2)
			    && ((equation[j+1].level == level && (j + 2) == e2)
			    || (equation[j+1].level == (level + 1)
			    && (j + 4) == e2
			    && equation[j+2].level == (level + 1)
			    && equation[j+2].token.operatr == DIVIDE
			    && equation[j+3].level == (level + 1)
			    && equation[j+3].kind == CONSTANT))
			    && equation[j-1].level == (level + 1)
			    && equation[j+1].kind == CONSTANT
			    && equation[j-1].kind == CONSTANT
			    && equation[j-2].level == (level + 1) && equation[j-2].token.operatr == POWER) {
				abs2_flag = true;
/*				return false;	*/
			}
#endif
			break;
		}
	}
#if	!ALWAYS_FACTOR_POWER
	if (abs1_flag && abs2_flag)
		return false;
#endif
	start2 = j;
#if	1
	if (se_compare(&equation[i+1], e1 - (i + 1), &one_token, 1, &diff_sign)) {
		return false;
	}
	if (se_compare(&equation[i+1], e1 - (i + 1), &equation[j+1], e2 - (j + 1), &diff_sign)) {
		b1 = i + 1;
		b2 = j + 1;
		i = e1;
		j = e2;
		goto common_exponent;
	}
#endif

fp_outer:
	pop1 = equation[i].token.operatr;
	if (pop1 == POWER)
		pop1 = TIMES;
	b1 = i + 1;
	if (b1 >= e1)
		return false;
	i = b1 + 1;
	for (; i < e1; i += 2) {
		if (equation[i].level == (level + 1)
		    && (equation[i].token.operatr == TIMES
		    || equation[i].token.operatr == DIVIDE)) {
			break;
		}
	}
	if (se_compare(&equation[b1], i - b1, &one_token, 1, &diff_sign)) {
		goto fp_outer;
	}
	j = start2;
fp_inner:
	b2 = j + 1;
	if (b2 >= e2)
		goto fp_outer;
	j = b2 + 1;
	for (; j < e2; j += 2) {
		if (equation[j].level == (level + 1)
		    && (equation[j].token.operatr == TIMES
		    || equation[j].token.operatr == DIVIDE)) {
			break;
		}
	}
	if (equation[b2-1].token.operatr == POWER) {
		if (pop1 != TIMES)
			goto fp_inner;
	} else if (equation[b2-1].token.operatr != pop1) {
		goto fp_inner;
	}
	if (se_compare(&equation[b1], i - b1, &equation[b2], j - b2, &diff_sign)) {
common_exponent:
		if (op2 == DIVIDE)
			diff_sign = !diff_sign;
		all_divide = (op1 == DIVIDE && diff_sign);
		len = 0;
		blt(&scratch[len], &equation[i1], (b1 - i1) * sizeof(token_type));
		len += b1 - i1;
		scratch[len].level = level + 1;
		scratch[len].kind = CONSTANT;
		if (!all_divide && op1 == DIVIDE) {
			scratch[len].token.constant = -1.0;
		} else {
			scratch[len].token.constant = 1.0;
		}
		len++;
		blt(&scratch[len], &equation[i], (e1 - i) * sizeof(token_type));
		len += e1 - i;
		for (k = 0; k < len; k++)
			scratch[k].level += 2;
		scratch[len].level = level + 1;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = TIMES;
		len++;
		k = len;
		blt(&scratch[len], &equation[i2], (b2 - i2) * sizeof(token_type));
		len += b2 - i2;
		scratch[len].level = level + 1;
		scratch[len].kind = CONSTANT;
		if (!all_divide && diff_sign) {
			scratch[len].token.constant = -1.0;
		} else {
			scratch[len].token.constant = 1.0;
		}
		len++;
		blt(&scratch[len], &equation[j], (e2 - j) * sizeof(token_type));
		len += e2 - j;
		for (; k < len; k++)
			scratch[k].level += 2;
		scratch[len].level = level;
		scratch[len].kind = OPERATOR;
		scratch[len].token.operatr = POWER;
		len++;
		if (pop1 == DIVIDE) {
			scratch[len].level = level + 1;
			scratch[len].kind = CONSTANT;
			scratch[len].token.constant = 1.0;
			len++;
			scratch[len].level = level + 1;
			scratch[len].kind = OPERATOR;
			scratch[len].token.operatr = DIVIDE;
			len++;
		}
		k = len;
		blt(&scratch[len], &equation[b1], (i - b1) * sizeof(token_type));
		len += i - b1;
		for (; k < len; k++)
			scratch[k].level++;
		if (*np + len - n1 - (n2 + 1) > n_tokens) {
			error_huge();
		}
		if (!all_divide && op1 == DIVIDE) {
			equation[i1-1].token.operatr = TIMES;
		}
		blt(&equation[i2-1], &equation[e2], (*np - e2) * sizeof(token_type));
		*np -= n2 + 1;
		blt(&equation[i1+len], &equation[e1], (*np - e1) * sizeof(token_type));
		*np += len - n1;
		blt(&equation[i1], scratch, len * sizeof(token_type));
		return true;
	}
	goto fp_inner;
}
