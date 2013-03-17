/*
 * Floating point complex number routines specifically for Mathomatic.
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
 * Convert doubles x and y from rectangular coordinates to polar coordinates.
 *
 * The amplitude is stored in *radiusp and the angle in radians is stored in *thetap.
 */
void
rect_to_polar(x, y, radiusp, thetap)
double	x, y, *radiusp, *thetap;
{
	*radiusp = sqrt(x * x + y * y);
	*thetap = atan2(y, x);
}

/*
 * The roots command.
 */
int
roots_cmd(cp)
char	*cp;
{
#define	MAX_ROOT	10000.0	/* Root limit needed because more roots become more inaccurate and take longer to check. */

	complexs	c, c2;
#if	!SILENT
	complexs	check;
	double		d;
#endif
	double		k, root, radius, theta, radius_root = 0.0;
	char		buf[MAX_CMD_LEN];

do_repeat:
	if (*cp == '\0') {
		my_strlcpy(prompt_str, _("Enter root (positive integer): "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL)
			return false;
	}
	root = strtod(cp, &cp);
	if ((*cp && *cp != ',' && !isspace(*cp)) || !isfinite(root) || root < 0.0 || root > MAX_ROOT || fmod(root, 1.0) != 0.0) {
		error(_("Root invalid or out of range."));
		printf(_("Root must be a positive integer less than or equal to %.0f.\n"), MAX_ROOT);
		return false;
	}
	cp = skip_comma_space(cp);
	if (*cp == '\0') {
		my_strlcpy(prompt_str, _("Enter real part (X): "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL)
			return false;
	}
	c.re = strtod(cp, &cp);
	if (*cp && *cp != ',' && !isspace(*cp)) {
		error(_("Number expected."));
		return false;
	}
	cp = skip_comma_space(cp);
	if (*cp == '\0') {
		my_strlcpy(prompt_str, _("Enter imaginary part (Y): "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL)
			return false;
	}
	c.im = strtod(cp, &cp);
	if (*cp) {
		error(_("Number expected."));
		return false;
	}
	if (c.re == 0.0 && c.im == 0.0) {
		return repeat_flag;
	}
/* convert to polar coordinates */
	errno = 0;
	rect_to_polar(c.re, c.im, &radius, &theta);
	if (root) {
		radius_root = pow(radius, 1.0 / root);
	}
	check_err();
	fprintf(gfp, _("\nThe polar coordinates are:\n%.*g amplitude and\n%.*g radians (%.*g degrees).\n\n"),
	    precision, radius, precision, theta, precision, theta * 180.0 / M_PI);
	if (root) {
		if (c.im == 0.0) {
			fprintf(gfp, _("The %.12g roots of (%.12g)^(1/%.12g) are:\n\n"), root, c.re, root);
		} else {
			fprintf(gfp, _("The %.12g roots of (%.12g%+.12g*i)^(1/%.12g) are:\n\n"), root, c.re, c.im, root);
		}
		for (k = 0.0; k < root; k += 1.0) {
/* add constants to theta and convert back to rectangular coordinates */
			c2.re = radius_root * cos((theta + 2.0 * k * M_PI) / root);
			c2.im = radius_root * sin((theta + 2.0 * k * M_PI) / root);
			complex_fixup(&c2);
			if (c2.re || c2.im == 0.0) {
				fprintf(gfp, "%.12g ", c2.re);
			}
			if (c2.im) {
				fprintf(gfp, "%+.12g*i", c2.im);
			}
			fprintf(gfp, "\n");
#if	!SILENT
			if (debug_level <= 0) {
				continue;
			}
			check = c2;
			for (d = 1.0; d < root; d += 1.0) {
				check = complex_mult(check, c2);
			}
			complex_fixup(&check);
			printf(_("Inverse check:"));
			if (check.re || check.im == 0.0) {
				printf(" %.10g", check.re);
			}
			if (check.im) {
				printf(" %+.10g*i", check.im);
			}
			printf("\n\n");
#endif
		}
	}
	if (repeat_flag)
		goto do_repeat;
	return true;
}

/*
 * Approximate roots of complex numbers in an equation side:
 * (complex^real) and (real^complex) and (complex^complex) all result in a complex number.
 * This only gives one root, even when there may be many roots.
 * Works best when the equation side has been approximated before this.
 *
 * Return true if the equation side was modified.
 */
int
complex_root_simp(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
	int		i, j;
	int		level;
	int		len;
	complexs	c, p, r;
	int		modified = false;

start_over:
	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr != POWER)
			continue;
		level = equation[i].level;
		for (j = i + 2; j < *np && equation[j].level >= level; j += 2)
			;
		len = j - (i + 1);
		if (!parse_complex(&equation[i+1], len, &p))
			continue;
		for (j = i - 1; j >= 0 && equation[j].level >= level; j--)
			;
		j++;
		if (!parse_complex(&equation[j], i - j, &c))
			continue;
		if (c.im == 0.0 && p.im == 0.0)
			continue;
		i += len + 1;
		r = complex_pow(c, p);

#if	0
		printf("(%.14g+%.14gi)^(%.14g+%.14gi) = %.14g+%.14gi\n", c.re, c.im, p.re, p.im, r.re, r.im);
#endif

		if (*np + 5 - (i - j) > n_tokens) {
			error_huge();
		}
		if ((j + 5) != i) {
			blt(&equation[j+5], &equation[i], (*np - i) * sizeof(token_type));
			*np += 5 - (i - j);
		}
		equation[j].level = level;
		equation[j].kind = CONSTANT;
		equation[j].token.constant = r.re;
		j++;
		equation[j].level = level;
		equation[j].kind = OPERATOR;
		equation[j].token.operatr = PLUS;
		j++;
		level++;
		equation[j].level = level;
		equation[j].kind = CONSTANT;
		equation[j].token.constant = r.im;
		j++;
		equation[j].level = level;
		equation[j].kind = OPERATOR;
		equation[j].token.operatr = TIMES;
		j++;
		equation[j].level = level;
		equation[j].kind = VARIABLE;
		equation[j].token.variable = IMAGINARY;
		modified = true;
		goto start_over;
	}
	if (modified) {
		debug_string(1, _("Complex number roots approximated."));
	}
	return modified;
}

/*
 * Approximate all roots of complex numbers in an equation side.
 *
 * Return true if anything was approximated.
 */
int
approximate_complex_roots(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
	int	rv = false;

	for (;;) {
		elim_loop(equation, np);
		if (!complex_root_simp(equation, np))
			break;
		rv = true;
	}
	return rv;
}

/*
 * Get a constant, if the passed expression evaluates to a constant.
 * This should not be called from low level routines.
 *
 * Return true if successful, with the floating point constant returned in *dp.
 */
int
get_constant(p1, n, dp)
token_type	*p1;	/* expression pointer */
int		n;	/* length of expression */
double		*dp;	/* pointer to returned double */
{
	int	i, j;
	int	level;
	double	d1, d2;
	int	prev_approx_flag;

#if	DEBUG
	if (n < 1 || (n & 1) != 1) {
		error_bug("Call to get_constant() has invalid expression length.");
	}
#endif
	if (n == 1) {
		switch (p1[0].kind) {
		case CONSTANT:
			*dp = p1[0].token.constant;
			return true;
		case VARIABLE:
			if (var_is_const(p1[0].token.variable, dp)) {
				return true;
			}
			break;
		case OPERATOR:
			break;
		}
	} else if (n >= 3) {
		level = p1[1].level;
		if (!get_constant(p1, 1, &d1))
			return false;
		for (i = 1; i < n; i = j) {
			if (p1[i].kind != OPERATOR || p1[i].level > level) {
#if	DEBUG
				error_bug("Possible error in get_constant().");
#endif
				return false;
			}
			level = p1[i].level;
			for (j = i + 2; j < n && p1[j].level > level; j += 2)
				;
			if (!get_constant(&p1[i+1], j - (i + 1), &d2))
				return false;
			prev_approx_flag = approximate_roots;
			approximate_roots = true;
			if (calc(NULL, &d1, p1[i].token.operatr, d2)) {
				approximate_roots = prev_approx_flag;
				if (p1[i].token.operatr == POWER && !domain_check)
					return false;
				domain_check = false;
			} else {
				approximate_roots = prev_approx_flag;
				domain_check = false;
				return false;
			}
		}
		*dp = d1;
		return true;
	}
	return false;
}

/*
 * Get the value of a constant complex number expression.
 * Doesn't always work unless expression is approximated first
 * with something like the approximate command.
 * Functionality was greatly improved recently, making success more likely
 * without approximating.
 *
 * If successful, return true with complex number in *cp.
 */
int
parse_complex(p1, n, cp)
token_type	*p1;	/* expression pointer */
int		n;	/* length of expression */
complexs	*cp;	/* pointer to returned complex number */
{
	int		j, k;
	int		imag_cnt = 0, times_cnt = 0;
	complexs	c, tmp;
	int		level, level2;

	if (!exp_is_numeric(p1, n)) {
		return false;
	}
	if (get_constant(p1, n, &c.re)) {
		c.im = 0.0;
		*cp = c;
		return true;
	}
	if (found_var(p1, n, IMAGINARY) != 1)
		return false;
	level = min_level(p1, n);
	c.re = 0.0;
	c.im = 1.0;
	j = n - 1;
	do {
		for (k = j - 1; k > 0 && p1[k].level > level; k -= 2)
			;
		if (k > 0) {
#if	DEBUG
			if (p1[k].level != level || p1[k].kind != OPERATOR) {
				error_bug("Error in parse_complex().");
			}
#endif
			switch (p1[k].token.operatr) {
			case MINUS:
			case PLUS:
				if (get_constant(&p1[k+1], j - k, &tmp.re)) {
					if (p1[k].token.operatr == MINUS)
						c.re -= tmp.re;
					else
						c.re += tmp.re;
					j = k - 1;
				}
			}
		} else
			break;
	} while (j < k);
	for (; j >= 0; j--) {
		switch (p1[j].kind) {
		case CONSTANT:
			break;
		case VARIABLE:
			if (var_is_const(p1[j].token.variable, NULL))
				break;
			if (p1[j].token.variable != IMAGINARY)
				return false;
			++imag_cnt;
			break;
		case OPERATOR:
			level2 = p1[j].level;
			switch (p1[j].token.operatr) {
			case TIMES:
			case DIVIDE:
				if (++times_cnt > 1)
					return false;
				if (level2 > (level + 1) || p1[j+1].level != level2)
					return false;
				for (k = j; k > 0 && p1[k].level == level2; k -= 2) {
					if (p1[k-1].level != level2)
						return false;
					if (!(p1[k+1].kind == VARIABLE && p1[k+1].token.variable == IMAGINARY)) {
						if (get_constant(&p1[k+1], 1, &tmp.im)) {
							if (p1[k].token.operatr == DIVIDE)
								c.im /= tmp.im;
							else
								c.im *= tmp.im;
						} else
							return false;
					} else if (p1[k].token.operatr == DIVIDE) {
						c.im = -c.im;
					}
					if (p1[k-1].kind == VARIABLE && p1[k-1].token.variable == IMAGINARY) {
						if (++imag_cnt > 1)
							return false;
						k -= 2;
						if (k > 0 && p1[k].level == level2) {
							if (p1[k-1].level != level2)
								return false;
							if (p1[k-1].kind == VARIABLE && p1[k-1].token.variable == IMAGINARY)
								return false;
							if (p1[k].token.operatr == DIVIDE)
								c.im = -c.im;
						} else
							break;
					}		
				}
				if (!(p1[k+1].kind == VARIABLE && p1[k+1].token.variable == IMAGINARY)) {
					if (get_constant(&p1[k+1], 1, &tmp.im)) {
						c.im *= tmp.im;
					} else
						return false;
				}
				j = k + 1;
				continue;
			case MINUS:
				if (imag_cnt) {
					c.im = -c.im;
				}
			case PLUS:
				if (level != level2)
					return false;
				if (get_constant(p1, j, &tmp.re)) {
					c.re += tmp.re;
					goto done;
				}
				return false;
			}
		default:
			return false;
		}
	}
done:
	if (imag_cnt != 1) {
#if	DEBUG
		error_bug("Imaginary count wrong in parse_complex().");
#else
		return false;
#endif
	}
	*cp = c;
	return true;
}
