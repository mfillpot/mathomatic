/*
 * Mathomatic unfactorizing (expanding) routines.
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

static int unf_sub(token_type *equation, int *np, int b1, int loc, int e1, int level, int ii);

/*
 * Unfactor times and divide only (products of sums) and simplify.
 *
 * Return true if equation side was unfactored.
 */
int
uf_tsimp(equation, np)
token_type	*equation;
int		*np;
{
	int	rv;

	rv = uf_times(equation, np);
	simp_loop(equation, np);
	while (uf_times(equation, np)) {
		rv = true;
		simp_loop(equation, np);
	}
	return rv;
}

/*
 * Unfactor power only.
 * (a * b)^c -> a^c * b^c
 *
 * Return true if equation side was unfactored.
 */
int
uf_power(equation, np)
token_type	*equation;
int		*np;
{
	int	count = -1;

	do {
		organize(equation, np);
		if (++count > 0)
			break;
	} while (sub_ufactor(equation, np, 2));
	return count;
}

/*
 * Unfactor power only.
 * a^(b + c) -> a^b * a^c
 *
 * Return true if equation side was unfactored.
 */
int
uf_pplus(equation, np)
token_type	*equation;
int		*np;
{
	int	count = -1;

	do {
		organize(equation, np);
		if (++count > 0)
			break;
	} while (sub_ufactor(equation, np, 4));
	return count;
}

/*
 * Unfactor all power operators.
 * Same as a call to uf_pplus() and uf_power(), only faster.
 */
void
uf_allpower(equation, np)
token_type	*equation;
int		*np;
{
	do {
		organize(equation, np);
	} while (sub_ufactor(equation, np, 0));
}

/*
 * Unfactor power only if it will help with expansion and exponent is <= 100.
 * (a + 1)^2 -> (a + 1)*(a + 1)
 * Afterwards we also sneak in simplifying division by irrational constants,
 * because it works best here.
 *
 * uf_times() is usually called after this to complete the expansion.
 */
void
uf_repeat(equation, np)
token_type	*equation;
int		*np;
{
	int	count = -1;

	do {
		organize(equation, np);
		if (++count > 0)
			break;
	} while (sub_ufactor(equation, np, 6));
	patch_root_div(equation, np);
}

/*
 * Unfactor power only.
 * a^2 -> a*a
 * Useful for removing all integer powers.
 */
void
uf_repeat_always(equation, np)
token_type	*equation;
int		*np;
{
	int	count = -1;

	do {
		organize(equation, np);
		if (++count > 0)
			break;
	} while (sub_ufactor(equation, np, 8));
}

/*
 * Totally unfactor equation side and simplify.
 */
void
uf_simp(equation, np)
token_type	*equation;	/* pointer to beginning of equation side */
int		*np;		/* pointer to length of equation side */
{
	uf_tsimp(equation, np);
	uf_power(equation, np);
	uf_repeat(equation, np);
	uf_tsimp(equation, np);
}

/*
 * Unfactor equation side and simplify.
 * Don't call uf_repeat().
 */
void
uf_simp_no_repeat(equation, np)
token_type	*equation;
int		*np;
{
	uf_power(equation, np);
	uf_tsimp(equation, np);
}

/*
 * Totally unfactor equation side with no simplification.
 */
int
ufactor(equation, np)
token_type	*equation;
int		*np;
{
	int	rv;

	uf_repeat(equation, np);
	rv = uf_times(equation, np);
	uf_allpower(equation, np);
	return rv;
}

/*
 * Increase the level of numerators by 2, so that the divide operator is not unfactored.
 */
static void
no_divide(equation, np)
token_type	*equation;
int		*np;
{
	int	i, j;
	int	level;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr == DIVIDE) {
			level = equation[i].level;
			for (j = i - 1; j >= 0; j--) {
				if (equation[j].level < level)
					break;
				equation[j].level += 2;
			}
		}
	}
}

/*
 * Unfactor times and divide only (products of sums like (a+b)*(c+d)).
 * (a + b)*c -> a*c + b*c
 * If partial_flag is true, don't expand (a+b)/(c+d) nor (a+b)/c.
 *
 * Return true if equation side was unfactored.
 */
int
uf_times(equation, np)
token_type	*equation;
int		*np;
{
	int	i;
	int	rv = false;

	do {
		organize(equation, np);
		if (reorder(equation, np)) {
			organize(equation, np);
		}
		group_proc(equation, np);
		if (partial_flag) {
			no_divide(equation, np);
		}
		i = sub_ufactor(equation, np, 1);
		rv |= i;
	} while (i);
	organize(equation, np);
	return rv;
}

/*
 * General equation side algebraic expansion routine.
 * Expands everything.
 * The type of expansion to be done is indicated by the code in "ii".
 *
 * Return true if equation side was modified.
 */
int
sub_ufactor(equation, np, ii)
token_type	*equation;
int		*np;
int		ii;
{
	int	modified = false;
	int	i;
	int	b1, e1;
	int	level;

	for (i = 1; i < *np; i += 2) {
		switch (equation[i].token.operatr) {
		case TIMES:
		case DIVIDE:
			if (ii == 1)
				break;
			else
				continue;
		case POWER:
			if ((ii & 1) == 0)	/* even number codes only */
				break;
		default:
			continue;
		}
		level = equation[i].level;
		for (b1 = i - 2; b1 >= 0; b1 -= 2)
			if (equation[b1].level < level)
				break;
		b1++;
		for (e1 = i + 2; e1 < *np; e1 += 2) {
			if (equation[e1].level < level)
				break;
		}
		if (unf_sub(equation, np, b1, i, e1, level, ii)) {
			modified = true;
			i = b1 - 1;
			continue;
		}
	}
	return modified;
}

static int
unf_sub(equation, np, b1, loc, e1, level, ii)
token_type	*equation;
int		*np;
int		b1, loc, e1, level;
int		ii;
{
	int		i, j, k;
	int		b2, eb1, be1;
	int		len;
	double		d1, d2;

	switch (equation[loc].token.operatr) {
	case TIMES:
	case DIVIDE:
		if (ii != 1)
			break;
		for (i = b1 + 1; i < e1; i += 2) {
			if (equation[i].level == level + 1) {
				switch (equation[i].token.operatr) {
				case PLUS:
				case MINUS:
					break;
				default:
					continue;
				}
				for (b2 = i - 2; b2 >= b1; b2 -= 2)
					if (equation[b2].level <= level)
						break;
				b2++;
				eb1 = b2;
				for (be1 = i + 2; be1 < e1; be1 += 2)
					if (equation[be1].level <= level)
						break;
				if (eb1 > b1 && equation[eb1-1].token.operatr == DIVIDE) {
					i = be1 - 2;
					continue;
				}
				len = 0;
u_again:
				if (len + (eb1 - b1) + (i - b2) + (e1 - be1) + 1 > n_tokens) {
					error_huge();
				}
				blt(&scratch[len], &equation[b1], (eb1 - b1) * sizeof(token_type));
				j = len;
				len += (eb1 - b1);
				for (; j < len; j++)
					scratch[j].level++;
				blt(&scratch[len], &equation[b2], (i - b2) * sizeof(token_type));
				len += (i - b2);
				blt(&scratch[len], &equation[be1], (e1 - be1) * sizeof(token_type));
				j = len;
				len += (e1 - be1);
				for (; j < len; j++)
					scratch[j].level++;
				if (i < be1) {
					scratch[len] = equation[i];
					scratch[len].level--;
					len++;
					b2 = i + 1;
					for (i += 2; i < be1; i += 2) {
						if (equation[i].level == (level + 1))
							break;
					}
					goto u_again;
				} else {
					if (*np - (e1 - b1) + len > n_tokens) {
						error_huge();
					}
					blt(&equation[b1+len], &equation[e1], (*np - e1) * sizeof(token_type));
					*np += len - (e1 - b1);
					blt(&equation[b1], scratch, len * sizeof(token_type));
					return true;
				}
			}
		}
		break;
	case POWER:
#if	!ALWAYS_UNFACTOR_POWER
/* avoid absolute values: */
		if ((loc + 3) < *np && equation[loc+1].level == level && equation[loc+1].kind == CONSTANT
		    && equation[loc+2].level == (level - 1) && equation[loc+2].token.operatr == POWER
		    && equation[loc+3].kind == CONSTANT && ((equation[loc+3].level == (level - 1))
		    || ((loc + 5) < *np && equation[loc+3].level == level 
		    && equation[loc+4].level == level && equation[loc+4].token.operatr == DIVIDE
		    && equation[loc+5].level == level && equation[loc+5].kind == CONSTANT
		    && ((loc + 6) >= *np || equation[loc+6].level < level)))) {
			return false;
		}
#endif
		if (ii == 2 || ii == 0) {
			for (i = b1 + 1; i < loc; i += 2) {
				if (equation[i].level != level + 1)
					continue;
				switch (equation[i].token.operatr) {
				case TIMES:
				case DIVIDE:
					break;
				default:
					goto do_pplus;
				}
				b2 = b1;
				len = 0;
u1_again:
				if (len + (i - b2) + (e1 - loc) + 1 > n_tokens) {
					error_huge();
				}
				blt(&scratch[len], &equation[b2], (i - b2) * sizeof(token_type));
				len += (i - b2);
				blt(&scratch[len], &equation[loc], (e1 - loc) * sizeof(token_type));
				j = len;
				len += (e1 - loc);
				for (; j < len; j++)
					scratch[j].level++;
				if (i < loc) {
					scratch[len] = equation[i];
					scratch[len].level--;
					len++;
					b2 = i + 1;
					for (i += 2; i < loc; i += 2) {
						if (equation[i].level == (level + 1))
							break;
					}
					goto u1_again;
				} else {
					if (*np - (e1 - b1) + len > n_tokens) {
						error_huge();
					}
					blt(&equation[b1+len], &equation[e1], (*np - e1) * sizeof(token_type));
					*np += len - (e1 - b1);
					blt(&equation[b1], scratch, len * sizeof(token_type));
					return true;
				}
			}
		}
do_pplus:
		if (ii == 4 || ii == 0) {
			for (i = loc + 2; i < e1; i += 2) {
				if (equation[i].level != level + 1)
					continue;
				switch (equation[i].token.operatr) {
				case PLUS:
				case MINUS:
					break;
				default:
					goto do_repeat;
				}
				b2 = loc + 1;
				len = 0;
u2_again:
				if (len + (loc - b1) + (i - b2) + 2 > n_tokens) {
					error_huge();
				}
				j = len;
				blt(&scratch[len], &equation[b1], (loc + 1 - b1) * sizeof(token_type));
				len += (loc + 1 - b1);
				for (; j < len; j++)
					scratch[j].level++;
				blt(&scratch[len], &equation[b2], (i - b2) * sizeof(token_type));
				len += (i - b2);
				if (i < e1) {
					scratch[len].level = level;
					scratch[len].kind = OPERATOR;
					if (equation[i].token.operatr == PLUS)
						scratch[len].token.operatr = TIMES;
					else
						scratch[len].token.operatr = DIVIDE;
					len++;
					b2 = i + 1;
					for (i += 2; i < e1; i += 2) {
						if (equation[i].level == (level + 1))
							break;
					}
					goto u2_again;
				} else {
					if (*np - (e1 - b1) + len > n_tokens) {
						error_huge();
					}
					blt(&equation[b1+len], &equation[e1], (*np - e1) * sizeof(token_type));
					*np += len - (e1 - b1);
					blt(&equation[b1], scratch, len * sizeof(token_type));
					return true;
				}
			}
		}
do_repeat:
		/* unfactor power: (a + 1)^2 -> (a + 1)*(a + 1) */
		if (ii != 6 && ii != 8)
			break;
		i = loc;
		if (equation[i+1].level != level || equation[i+1].kind != CONSTANT)
			break;
		d1 = equation[i+1].token.constant;
		if (!isfinite(d1) || d1 <= 1.0)
			break;
		if (ii != 8) {	/* if true, only do useful expansions */
			if (d1 > 100.0)	/* ignore exponents over 100 */
				break;
			if ((i - b1) == 1 && equation[b1].kind != CONSTANT)
				break;
			if ((i - b1) > 1 && d1 > 2.0 && fmod(d1, 1.0) != 0.0)
				break;
		}
		d1 = ceil(d1) - 1.0;
		d2 = d1 * ((i - b1) + 1.0);
		if ((*np + d2) > (n_tokens - 10)) {
			break;	/* too big to expand, do nothing */
		}
		j = d1;
		k = d2;
		blt(&equation[e1+k], &equation[e1], (*np - e1) * sizeof(token_type));
		*np += k;
		equation[i+1].token.constant -= d1;
		k = e1;
		while (j-- > 0) {
			equation[k].level = level;
			equation[k].kind = OPERATOR;
			equation[k].token.operatr = TIMES;
			blt(&equation[k+1], &equation[b1], (i - b1) * sizeof(token_type));
			k += (i - b1) + 1;
		}
		if (equation[i+1].token.constant == 1.0) {
			blt(&equation[i], &equation[e1], (*np - e1) * sizeof(token_type));
			*np -= (e1 - i);
		} else {
			for (j = b1; j < e1; j++)
				equation[j].level++;
		}
		return true;
	}
	return false;
}

static int
usp_sub(equation, np, i)
token_type	*equation;
int		*np, i;
{
	int	level;
	int	j;

	level = equation[i].level;
	for (j = i - 2;; j -= 2) {
		if (j < 0)
			return false;
		if (equation[j].level < level) {
			if (equation[j].level == (level - 1) && equation[j].token.operatr == DIVIDE) {
				break;
			} else
				return false;
		}
	}
	if ((*np + 2) > n_tokens) {
		error_huge();
	}
	equation[j].token.operatr = TIMES;
	for (j = i + 1;; j++) {
		if (j >= *np || equation[j].level < level)
			break;
		equation[j].level++;
	}
	i++;
	blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
	*np += 2;
	equation[i].level = level + 1;
	equation[i].kind = CONSTANT;
	equation[i].token.constant = -1.0;
	i++;
	equation[i].level = level + 1;
	equation[i].kind = OPERATOR;
	equation[i].token.operatr = TIMES;
	return true;
}

/*
 * Convert a/(x^y) to a*x^(-1*y).
 * If y is a constant, don't do.
 *
 * Return true if equation side is modified.
 */
int
unsimp_power(equation, np)
token_type	*equation;
int		*np;
{
	int	modified = false;
	int	i;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr == POWER) {
			if (equation[i].level == equation[i+1].level
			    && equation[i+1].kind == CONSTANT)
				continue;
			modified |= usp_sub(equation, np, i);
		}
	}
	return modified;
}

#if	0	/* the following is not currently used */
/*
 * Convert a/(x^y) to a*(1/x)^y.
 * If y is a constant, don't do.
 *
 * Return true if equation side is modified.
 */
int
unsimp2_power(equation, np)
token_type	*equation;
int		*np;
{
	int	modified = false;
	int	i;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr == POWER) {
			modified |= usp2_sub(equation, np, i);
		}
	}
	return modified;
}

int
usp2_sub(equation, np, i)
token_type	*equation;
int		*np, i;
{
	int	level;
	int	j, k;

	level = equation[i].level;
	if (equation[i+1].level == level && equation[i+1].kind == CONSTANT) {
		return false;
	}
	for (j = i - 2;; j -= 2) {
		if (j < 0)
			return false;
		if (equation[j].level < level) {
			if (equation[j].level == (level - 1) && equation[j].token.operatr == DIVIDE) {
				break;
			} else
				return false;
		}
	}
	if ((*np + 2) > n_tokens) {
		error_huge();
	}
	equation[j].token.operatr = TIMES;
	j++;
	for (k = j; k < i; k++) {
		equation[k].level++;
	}
	blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
	*np += 2;
	equation[j].level = level + 1;
	equation[j].kind = CONSTANT;
	equation[j].token.constant = 1.0;
	j++;
	equation[j].level = level + 1;
	equation[j].kind = OPERATOR;
	equation[j].token.operatr = DIVIDE;
	return true;
}
#endif

/*
 * Convert anything times a negative constant to a positive constant
 * divided by -1.
 * When uf_times() is done after this, the additive denominators will be
 * attempted to be negated, possibly getting rid of unneeded times -1.
 */
void
uf_neg_help(equation, np)
token_type	*equation;
int		*np;
{
	int	i;
	int	level;

	for (i = 0; i < *np - 1; i += 2) {
		if (equation[i].kind == CONSTANT && equation[i].token.constant < 0.0) {
			level = equation[i].level;
			if (equation[i+1].level == level) {
				switch (equation[i+1].token.operatr) {
				case TIMES:
				case DIVIDE:
					if ((*np + 2) > n_tokens) {
						error_huge();
					}
					blt(&equation[i+3], &equation[i+1], (*np - (i + 1)) * sizeof(token_type));
					*np += 2;
					equation[i].token.constant = -equation[i].token.constant;
					i++;
					equation[i].level = level;
					equation[i].kind = OPERATOR;
					equation[i].token.operatr = DIVIDE;
					i++;
					equation[i].level = level;
					equation[i].kind = CONSTANT;
					equation[i].token.constant = -1.0;
					break;
				}
			}
		}
	}
}

/*
 * Simplify division by irrational constants (roots like 2^.5) by
 * converting 1/(k1^k2) to 1/(k1^(k2-1))/k1 if k1 is integer,
 * otherwise convert 1/(k1^k2) to 1*((1/k1)^k2).
 *
 * Return true if equation side was modified.
 */
int
patch_root_div(equation, np)
token_type	*equation;
int		*np;
{
	int	i;
	int	level;
	int	modified = false;

	for (i = 1; i < *np - 2; i += 2) {
		if (equation[i].token.operatr == DIVIDE) {
			level = equation[i].level + 1;
			if (equation[i+2].token.operatr == POWER
			    && equation[i+2].level == level
			    && equation[i+1].kind == CONSTANT
			    && equation[i+3].level == level
			    && equation[i+3].kind == CONSTANT) {
				if (fmod(equation[i+1].token.constant, 1.0) == 0.0) {
					if (!rationalize_denominators
					    || !isfinite(equation[i+3].token.constant)
					    || equation[i+3].token.constant <= 0.0
					    || equation[i+3].token.constant >= 1.0) {
						continue;
					}
					if (*np + 2 > n_tokens)
						error_huge();
					equation[i+3].token.constant -= 1.0;
					blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
					*np += 2;
					i++;
					equation[i].level = level - 1;
					equation[i].kind = CONSTANT;
					equation[i].token.constant = equation[i+2].token.constant;
					i++;
				} else {
					equation[i].token.operatr = TIMES;
					equation[i+1].token.constant = 1.0 / equation[i+1].token.constant;
				}
				modified = true;
			}
		}
	}
	return modified;
}
