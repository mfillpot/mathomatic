/*
 * Mathomatic integration routines and commands.
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

static int integrate_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v);
static int laplace_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v);
static int inv_laplace_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v);

//static int	constant_var_number = 1;	/* makes unique numbers for the constant of integration */

/*
 * Make variable "v" always raised to a power,
 * unless it is on the right side of a power operator.
 */
void
make_powers(MathoMatic* mathomatic, token_type *equation, int *np, long v)
//token_type	*equation;	/* pointer to beginning of equation side */
//int		*np;		/* pointer to length of equation side */
//long		v;		/* Mathomatic variable */
{
	int	i;
	int	level;

	for (i = 0; i < *np;) {
		level = equation[i].level;
		if (equation[i].kind == OPERATOR && equation[i].token.operatr == POWER) {
			for (i += 2; i < *np && equation[i].level >= level; i += 2)
				;
			continue;
		}
		if (equation[i].kind == VARIABLE && equation[i].token.variable == v) {
			if ((i + 1) >= *np || equation[i+1].token.operatr != POWER) {
				if (*np + 2 > mathomatic->n_tokens) {
					error_huge(mathomatic);
				}
				level++;
				equation[i].level = level;
				i++;
				blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
				*np += 2;
				equation[i].level = level;
				equation[i].kind = OPERATOR;
				equation[i].token.operatr = POWER;
				i++;
				equation[i].level = level;
				equation[i].kind = CONSTANT;
				equation[i].token.constant = 1.0;
			}
		}
		i++;
	}
}

/*
 * Integration dispatch routine for polynomials.
 * Handles the level 1 additive operators,
 * sending each polynomial term to the specified integration function.
 *
 * Return true if successful.
 */
int
int_dispatch(MathoMatic* mathomatic, token_type *equation, int *np, long v, int	(*func)(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v))
//token_type	*equation;	/* pointer to beginning of equation side to integrate */
//int		*np;		/* pointer to length of equation side */
//long		v;		/* integration variable */
//int		(*func)(token_type *equation, int *np, int loc, int eloc, long v);	/* integration function to call for each term */
{
	int	i, j;

	make_powers(mathomatic, equation, np, v);
	for (j = 0, i = 1;; i += 2) {
		if (i >= *np) {
			return((*func)(mathomatic, equation, np, j, i, v));
		}
		if (equation[i].level == 1
		    && (equation[i].token.operatr == PLUS || equation[i].token.operatr == MINUS)) {
			if (!(*func)(mathomatic, equation, np, j, i, v)) {
				return false;
			}
			for (i = j + 1;; i += 2) {
				if (i >= *np) {
					return true;
				}
				if (equation[i].level == 1) {
					j = i + 1;
					break;
				}
			}
		}
	}
	return true;
}

/*
 * Do the actual integration of a polynomial term.
 *
 * Return true if successful.
 */
static int
integrate_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v)
//token_type	*equation;	/* pointer to beginning of equation side */
//int		*np;		/* pointer to length of equation side */
//int		loc;		/* beginning location of term */
//int		eloc;		/* end location of term */
//long		v;		/* variable of integration */
{
	int		i, j, k;
	int		len;
	int		level, vlevel, mlevel;
	int		count;
	int		div_flag;

	level = min_level(mathomatic, &equation[loc], eloc - loc);
	/* determine if the term is a polynomial term in "v" */
	for (i = loc, count = 0; i < eloc; i += 2) {
		if (equation[i].kind == VARIABLE && equation[i].token.variable == v) {
			count++;
			if (count > 1)
				return false;
			vlevel = equation[i].level;
			if (vlevel == level || vlevel == (level + 1)) {
				for (k = loc + 1; k < eloc; k += 2) {
					if (equation[k].level == level) {
						switch (equation[k].token.operatr) {
						case DIVIDE:
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
					if ((i + 1) < eloc && equation[i+1].level == vlevel
					    && equation[i+1].token.operatr == POWER) {
						continue;
					}
				} else {
					continue;
				}
			}
			return false;
		}
	}
	mlevel = level + 1;
	for (j = loc; j < eloc; j++)
		equation[j].level += 2;
	for (i = loc; i < eloc; i += 2) {
		if (equation[i].kind == VARIABLE && equation[i].token.variable == v) {
			div_flag = (i > loc && equation[i-1].token.operatr == DIVIDE);
			i++;
			if (i >= eloc || equation[i].token.operatr != POWER)
				return false;
			level = equation[i].level;
			i++;
			if (div_flag) {
				if (equation[i].level == level
				    && equation[i].kind == CONSTANT
				    && equation[i].token.constant == 1.0)
					return false;
				if (*np + 2 > mathomatic->n_tokens)
					error_huge(mathomatic);
				for (j = i; j < eloc && equation[j].level >= level; j++)
					equation[j].level++;
				equation[i-3].token.operatr = TIMES;
				blt(&equation[i+2], &equation[i], (*np - i) * sizeof(token_type));
				*np += 2;
				eloc += 2;
				equation[i].level = level + 1;
				equation[i].kind = CONSTANT;
				equation[i].token.constant = -1.0;
				equation[i+1].level = level + 1;
				equation[i+1].kind = OPERATOR;
				equation[i+1].token.operatr = TIMES;
			}
			for (j = i; j < eloc && equation[j].level >= level; j++)
				equation[j].level++;
			len = j - i;
			if (*np + len + 5 > mathomatic->n_tokens)
				error_huge(mathomatic);
			blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
			*np += 2;
			eloc += 2;
			len += 2;
			level++;
			equation[j].level = level;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = PLUS;
			j++;
			equation[j].level = level;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = 1.0;
			blt(&equation[eloc+len+1], &equation[eloc], (*np - eloc) * sizeof(token_type));
			*np += len + 1;
			equation[eloc].level = mlevel;
			equation[eloc].kind = OPERATOR;
			equation[eloc].token.operatr = DIVIDE;
			blt(&equation[eloc+1], &equation[i], len * sizeof(token_type));
			return true;
		}
	}
	if (*np + 2 > mathomatic->n_tokens) {
		error_huge(mathomatic);
	}
	blt(&equation[eloc+2], &equation[eloc], (*np - eloc) * sizeof(token_type));
	*np += 2;
	equation[eloc].level = mlevel;
	equation[eloc].kind = OPERATOR;
	equation[eloc].token.operatr = TIMES;
	eloc++;
	equation[eloc].level = mlevel;
	equation[eloc].kind = VARIABLE;
	equation[eloc].token.variable = v;
	return true;
}

/*
 * The integrate command.
 */
int
integrate_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i, j;
	int		len;
	long		v = 0;
	token_type	*source, *dest;
	int		n1, n2, *nps, *np;
	int		definite_flag = false, constant_flag = false, solved;
	double		integrate_order = 1.0;
	char		var_name_buf[MAX_VAR_LEN], *cp_start;
	long		l1;

	cp_start = cp;
	if (current_not_defined(mathomatic)) {
		return false;
	}
	mathomatic->n_tlhs = 0;
	mathomatic->n_trhs = 0;
	solved = solved_equation(mathomatic, mathomatic->cur_equation);
	i = next_espace(mathomatic);
	for (;; cp = skip_param(cp)) {
		if (strcmp_tospace(cp, "definite") == 0) {
			definite_flag = true;
			continue;
		}
		if (strcmp_tospace(cp, "constant") == 0) {
			constant_flag = true;
			continue;
		}
		break;
	}
	if (constant_flag && definite_flag) {
		error(mathomatic, _("Conflicting options given."));
		return false;
	}
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		if (!solved) {
			warning(mathomatic, _("Not a solved equation."));
		}
		debug_string(mathomatic, 0, _("Only the RHS will be transformed."));
		source = mathomatic->rhs[mathomatic->cur_equation];
		nps = &mathomatic->n_rhs[mathomatic->cur_equation];
		dest = mathomatic->rhs[i];
		np = &mathomatic->n_rhs[i];
	} else {
		source = mathomatic->lhs[mathomatic->cur_equation];
		nps = &mathomatic->n_lhs[mathomatic->cur_equation];
		dest = mathomatic->lhs[i];
		np = &mathomatic->n_lhs[i];
	}
	if (*cp) {
		if (isvarchar(mathomatic, *cp)) {
			cp = parse_var2(mathomatic, &v, cp);
			if (cp == NULL) {
				return false;
			}
		}
		if (*cp) {
			integrate_order = strtod(cp, &cp);
		}
		if (!isfinite(integrate_order) || integrate_order <= 0 || fmod(integrate_order, 1.0) != 0.0) {
			error(mathomatic, _("The order must be a positive integer."));
			return false;
		}
	}

	if (*cp) {
		cp = skip_comma_space(cp);
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, cp, false);
		if (cp == NULL || mathomatic->n_tlhs <= 0) {
			return false;
		}
	}
	if (*cp) {
		cp_start = cp;
		cp = skip_comma_space(cp);
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, cp, false);
		if (cp == NULL || extra_characters(mathomatic, cp) || mathomatic->n_trhs <= 0) {
			return false;
		}
	}
	mathomatic->show_usage = false;
	if (v == 0) {
		if (!prompt_var(mathomatic, &v)) {
			return false;
		}
	}
#if	!SILENT
	list_var(mathomatic, v, 0);
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		fprintf(mathomatic->gfp, _("Integrating the RHS with respect to %s"), mathomatic->var_str);
	} else {
		fprintf(mathomatic->gfp, _("Integrating with respect to %s"), mathomatic->var_str);
	}
	if (integrate_order != 1.0) {
		fprintf(mathomatic->gfp, _(" %.*g times"), mathomatic->precision, integrate_order);
	}
	fprintf(mathomatic->gfp, _(" and simplifying...\n"));
#endif
	mathomatic->partial_flag = false;
	uf_simp(mathomatic, source, nps);
	mathomatic->partial_flag = true;
	factorv(mathomatic, source, nps, v);
	blt(dest, source, *nps * sizeof(token_type));
	n1 = *nps;
	for (l1 = 0; l1 < integrate_order; l1++) {
		if (!int_dispatch(mathomatic, dest, &n1, v, integrate_sub)) {
			error(mathomatic, _("Integration failed, not a polynomial."));
			return false;
		}
		if (constant_flag) {
			if (n1 + 2 > mathomatic->n_tokens) {
				error_huge(mathomatic);
			}
			for (j = 0; j < n1; j++) {
				dest[j].level++;
			}
			dest[n1].kind = OPERATOR;
			dest[n1].level = 1;
			dest[n1].token.operatr = PLUS;
			n1++;
			dest[n1].kind = VARIABLE;
			dest[n1].level = 1;
			snprintf(var_name_buf, sizeof(var_name_buf), "C_%d", mathomatic->constant_var_number);
			if (parse_var(mathomatic, &dest[n1].token.variable, var_name_buf) == NULL) {
				return false;
			}
			n1++;
			mathomatic->constant_var_number++;
			if (mathomatic->constant_var_number < 0) {
				mathomatic->constant_var_number = 1;
			}
		}
		simp_loop(mathomatic, dest, &n1);
	}
	if (definite_flag) {
		if (mathomatic->n_tlhs == 0) {
			my_strlcpy(mathomatic->prompt_str, _("Enter lower bound: "), sizeof(mathomatic->prompt_str));
			if (!get_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs)) {
				return false;
			}
		}
		if (mathomatic->n_trhs == 0) {
			my_strlcpy(mathomatic->prompt_str, _("Enter upper bound: "), sizeof(mathomatic->prompt_str));
			if (!get_expr(mathomatic, mathomatic->trhs, &mathomatic->n_trhs)) {
				return false;
			}
		}
		blt(mathomatic->scratch, dest, n1 * sizeof(token_type));
		n2 = n1;
		subst_var_with_exp(mathomatic, mathomatic->scratch, &n2, mathomatic->tlhs, mathomatic->n_tlhs, v);
		subst_var_with_exp(mathomatic, dest, &n1, mathomatic->trhs, mathomatic->n_trhs, v);
		if (n1 + 1 + n2 > mathomatic->n_tokens) {
			error_huge(mathomatic);
		}
		for (j = 0; j < n1; j++) {
			dest[j].level++;
		}
		for (j = 0; j < n2; j++) {
			mathomatic->scratch[j].level++;
		}
		dest[n1].kind = OPERATOR;
		dest[n1].level = 1;
		dest[n1].token.operatr = MINUS;
		n1++;
		blt(&dest[n1], mathomatic->scratch, n2 * sizeof(token_type));
		n1 += n2;
	}
	simpa_side(mathomatic, dest, &n1, false, false);
	*np = n1;
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		blt(mathomatic->lhs[i], mathomatic->lhs[mathomatic->cur_equation], mathomatic->n_lhs[mathomatic->cur_equation] * sizeof(token_type));
		mathomatic->n_lhs[i] = mathomatic->n_lhs[mathomatic->cur_equation];
		if (solved && isvarchar(mathomatic, '\'')) {
			len = list_var(mathomatic, mathomatic->lhs[i][0].token.variable, 0);
			for (l1 = 0; l1 < integrate_order && len > 0 && mathomatic->var_str[len-1] == '\''; l1++) {
				mathomatic->var_str[--len] = '\0';
			}
			parse_var(mathomatic, &mathomatic->lhs[i][0].token.variable, mathomatic->var_str);
		}
	}
	mathomatic->cur_equation = i;
	return return_result(mathomatic, mathomatic->cur_equation);
}

/*
 * Do the actual Laplace transformation of a polynomial term.
 *
 * Return true if successful.
 */
static int
laplace_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v)
{
	int		i, j, k;
	int		len;
	int		level, mlevel;

	mlevel = min_level(mathomatic, &equation[loc], eloc - loc) + 1;
	for (j = loc; j < eloc; j++)
		equation[j].level += 2;
	for (i = loc; i < eloc; i += 2) {
		if (equation[i].kind == VARIABLE && equation[i].token.variable == v) {
			i++;
			if (i >= eloc || equation[i].token.operatr != POWER)
				return false;
			level = equation[i].level;
			i++;
			for (j = i; j < eloc && equation[j].level >= level; j++)
				equation[j].level++;
			len = j - i;
			if (*np + len + 7 > mathomatic->n_tokens)
				error_huge(mathomatic);
			blt(&equation[j+4], &equation[j], (*np - j) * sizeof(token_type));
			*np += 4;
			eloc += 4;
			level++;
			equation[j].level = level;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = PLUS;
			j++;
			equation[j].level = level;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = 1.0;
			j++;
			for (k = i; k < j; k++)
				equation[k].level++;
			equation[j].level = level;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = TIMES;
			j++;
			equation[j].level = level;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = -1.0;
			blt(&equation[eloc+len+3], &equation[eloc], (*np - eloc) * sizeof(token_type));
			*np += len + 3;
			k = eloc;
			equation[k].level = mlevel;
			equation[k].kind = OPERATOR;
			equation[k].token.operatr = TIMES;
			k++;
			blt(&equation[k], &equation[i], len * sizeof(token_type));
			k += len;
			equation[k].level = mlevel + 1;
			equation[k].kind = OPERATOR;
			equation[k].token.operatr = FACTORIAL;
			k++;
			equation[k].level = mlevel + 1;
			equation[k].kind = CONSTANT;
			equation[k].token.constant = 1.0;
			return true;
		}
	}
	if (*np + 2 > mathomatic->n_tokens) {
		error_huge(mathomatic);
	}
	blt(&equation[eloc+2], &equation[eloc], (*np - eloc) * sizeof(token_type));
	*np += 2;
	equation[eloc].level = mlevel;
	equation[eloc].kind = OPERATOR;
	equation[eloc].token.operatr = DIVIDE;
	eloc++;
	equation[eloc].level = mlevel;
	equation[eloc].kind = VARIABLE;
	equation[eloc].token.variable = v;
	return true;
}

/*
 * Do the actual inverse Laplace transformation of a polynomial term.
 *
 * Return true if successful.
 */
static int
inv_laplace_sub(MathoMatic* mathomatic, token_type *equation, int *np, int loc, int eloc, long v)
{
	int	i, j, k;
	int	len;
	int	level, mlevel;

	mlevel = min_level(mathomatic, &equation[loc], eloc - loc) + 1;
	for (j = loc; j < eloc; j++)
		equation[j].level += 2;
	for (i = loc; i < eloc; i += 2) {
		if (equation[i].kind == VARIABLE && equation[i].token.variable == v) {
			i++;
			if (i >= eloc || equation[i].token.operatr != POWER)
				return false;
			if ((i - 2) <= loc || equation[i-2].token.operatr != DIVIDE)
				return false;
			level = equation[i].level;
			i++;
			for (j = i; j < eloc && equation[j].level >= level; j++)
				equation[j].level++;
			len = j - i;
			if (*np + len + 7 > mathomatic->n_tokens)
				error_huge(mathomatic);
			equation[i-3].token.operatr = TIMES;
			blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
			*np += 2;
			eloc += 2;
			len += 2;
			level++;
			equation[j].level = level;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = MINUS;
			j++;
			equation[j].level = level;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = 1.0;
			blt(&equation[eloc+len+3], &equation[eloc], (*np - eloc) * sizeof(token_type));
			*np += len + 3;
			k = eloc;
			equation[k].level = mlevel;
			equation[k].kind = OPERATOR;
			equation[k].token.operatr = DIVIDE;
			k++;
			blt(&equation[k], &equation[i], len * sizeof(token_type));
			k += len;
			equation[k].level = mlevel + 1;
			equation[k].kind = OPERATOR;
			equation[k].token.operatr = FACTORIAL;
			k++;
			equation[k].level = mlevel + 1;
			equation[k].kind = CONSTANT;
			equation[k].token.constant = 1.0;
			return true;
		}
	}
	return false;
}

/*
 * The laplace command.
 */
int
laplace_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i;
	long		v = 0;
	int		inverse_flag, solved;
	token_type	*source, *dest;
	int		n1, *nps, *np;

	if (current_not_defined(mathomatic)) {
		return false;
	}
	solved = solved_equation(mathomatic, mathomatic->cur_equation);
	i = next_espace(mathomatic);
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		if (!solved) {
			warning(mathomatic, _("Not a solved equation."));
		}
#if	!SILENT
		fprintf(mathomatic->gfp, _("Only the RHS will be transformed.\n"));
#endif
		source = mathomatic->rhs[mathomatic->cur_equation];
		nps = &mathomatic->n_rhs[mathomatic->cur_equation];
		dest = mathomatic->rhs[i];
		np = &mathomatic->n_rhs[i];
	} else {
		source = mathomatic->lhs[mathomatic->cur_equation];
		nps = &mathomatic->n_lhs[mathomatic->cur_equation];
		dest = mathomatic->lhs[i];
		np = &mathomatic->n_lhs[i];
	}
	inverse_flag = (strcmp_tospace(cp, "inverse") == 0);
	if (inverse_flag) {
		cp = skip_param(cp);
	}
	if (*cp) {
		cp = parse_var2(mathomatic, &v, cp);
		if (cp == NULL) {
			return false;
		}
		if (extra_characters(mathomatic, cp)) {
			return false;
		}
	}
	mathomatic->show_usage = false;
	if (v == 0) {
		if (!prompt_var(mathomatic, &v)) {
			return false;
		}
	}
	mathomatic->partial_flag = false;
	uf_simp(mathomatic, source, nps);
	mathomatic->partial_flag = true;
	factorv(mathomatic, source, nps, v);
	blt(dest, source, *nps * sizeof(token_type));
	n1 = *nps;
	if (inverse_flag) {
		if (!poly_in_v(mathomatic, dest, n1, v, true) || !int_dispatch(mathomatic, dest, &n1, v, inv_laplace_sub)) {
			error(mathomatic, _("Inverse Laplace transformation failed."));
			return false;
		}
	} else {
		if (!poly_in_v(mathomatic, dest, n1, v, false) || !int_dispatch(mathomatic, dest, &n1, v, laplace_sub)) {
			error(mathomatic, _("Laplace transformation failed, not a polynomial."));
			return false;
		}
	}
#if	1
	simp_loop(mathomatic, dest, &n1);
#else
	simpa_side(mathomatic, dest, &n1, false, false);
#endif
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		blt(mathomatic->lhs[i], mathomatic->lhs[mathomatic->cur_equation], mathomatic->n_lhs[mathomatic->cur_equation] * sizeof(token_type));
		mathomatic->n_lhs[i] = mathomatic->n_lhs[mathomatic->cur_equation];
	}
	*np = n1;
	mathomatic->cur_equation = i;
	return return_result(mathomatic, mathomatic->cur_equation);
}

/*
 * Numerical integrate command.
 */
int
nintegrate_cmd(MathoMatic* mathomatic, char *cp)
{
	long		v = 0;			/* Mathomatic variable */
	int		i, j, k, i1, i2;
	int		len;
	int		level;
	int		iterations = 1000;	/* must be even */
	int		first_size = 0;
	int		trap_flag, singularity, solved;
	token_type	*ep, *source, *dest;
	int		n1, *nps, *np;
	char		*cp_start;

	cp_start = cp;
	if (current_not_defined(mathomatic)) {
		return false;
	}
	mathomatic->n_tlhs = 0;
	mathomatic->n_trhs = 0;
	solved = solved_equation(mathomatic, mathomatic->cur_equation);
	i = next_espace(mathomatic);
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		if (!solved) {
			warning(mathomatic, _("Not a solved equation."));
		}
		source = mathomatic->rhs[mathomatic->cur_equation];
		nps = &mathomatic->n_rhs[mathomatic->cur_equation];
		dest = mathomatic->rhs[i];
		np = &mathomatic->n_rhs[i];
	} else {
		source = mathomatic->lhs[mathomatic->cur_equation];
		nps = &mathomatic->n_lhs[mathomatic->cur_equation];
		dest = mathomatic->lhs[i];
		np = &mathomatic->n_lhs[i];
	}
	trap_flag = (strncasecmp(cp, "trap", 4) == 0);
	if (trap_flag) {
		cp = skip_param(cp);
	}
	if (*cp) {
		cp = parse_var2(mathomatic, &v, cp);
		if (cp == NULL) {
			return false;
		}
		if (*cp) {
			iterations = decstrtol(cp, &cp);
		}
		if (iterations <= 0 || (iterations % 2) != 0) {
			error(mathomatic, _("Number of partitions must be a positive, even integer."));
			return false;
		}
	}
	if (*cp) {
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, cp, false);
		if (cp == NULL || mathomatic->n_tlhs <= 0) {
			return false;
		}
	}
	if (*cp) {
		cp_start = cp;
		cp = skip_comma_space(cp);
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, cp, false);
		if (cp == NULL || extra_characters(mathomatic, cp) || mathomatic->n_trhs <= 0) {
			return false;
		}
	}
	mathomatic->show_usage = false;
	if (v == 0) {
		if (!prompt_var(mathomatic, &v)) {
			return false;
		}
	}
#if	!SILENT
	list_var(mathomatic, v, 0);
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		fprintf(mathomatic->gfp, _("Numerically integrating the RHS with respect to %s...\n"), mathomatic->var_str);
	} else {
		fprintf(mathomatic->gfp, _("Numerically integrating with respect to %s...\n"), mathomatic->var_str);
	}
#endif
	singularity = false;
	for (j = 1; j < *nps; j += 2) {
		if (source[j].token.operatr == DIVIDE) {
			for (k = j + 1; k < *nps && source[k].level >= source[j].level; k++) {
				if (source[k].kind == VARIABLE && source[k].token.variable == v) {
					singularity = true;
				}
				if (source[k].kind == OPERATOR && source[k].level == source[j].level)
					break;
			}
		}
	}
	if (singularity) {
		warning(mathomatic, _("Singularity detected, result of numerical integration might be wrong."));
	}
	if (mathomatic->n_tlhs == 0) {
		my_strlcpy(mathomatic->prompt_str, _("Enter lower bound: "), sizeof(mathomatic->prompt_str));
		if (!get_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs)) {
			return false;
		}
	}
	subst_constants(mathomatic->tlhs, &mathomatic->n_tlhs);
	simp_loop(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
	if (exp_contains_infinity(mathomatic->tlhs, mathomatic->n_tlhs)) {
		error(mathomatic, _("Not computable because: Lower bound contains infinity or NaN."));
		return false;
	}
	if (mathomatic->n_trhs == 0) {
		my_strlcpy(mathomatic->prompt_str, _("Enter upper bound: "), sizeof(mathomatic->prompt_str));
		if (!get_expr(mathomatic, mathomatic->trhs, &mathomatic->n_trhs)) {
			return false;
		}
	}
	subst_constants(mathomatic->trhs, &mathomatic->n_trhs);
	simp_loop(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
	if (exp_contains_infinity(mathomatic->trhs, mathomatic->n_trhs)) {
		error(mathomatic, _("Not computable because: Upper bound contains infinity or NaN."));
		return false;
	}
	if ((mathomatic->n_tlhs + mathomatic->n_trhs + 3) > mathomatic->n_tokens) {
		error_huge(mathomatic);
	}
#if	!SILENT
	fprintf(mathomatic->gfp, _("Approximating the definite integral\n"));
	if (trap_flag) {
		fprintf(mathomatic->gfp, _("using the trapezoid method (%d partitions)...\n"), iterations);
	} else {
		fprintf(mathomatic->gfp, _("using Simpson's rule (%d partitions)...\n"), iterations);
	}
#endif
	subst_constants(source, nps);
	simp_loop(mathomatic, source, nps);
	for (j = 0; j < mathomatic->n_trhs; j++) {
		mathomatic->trhs[j].level += 2;
	}
	mathomatic->trhs[mathomatic->n_trhs].level = 2;
	mathomatic->trhs[mathomatic->n_trhs].kind = OPERATOR;
	mathomatic->trhs[mathomatic->n_trhs].token.operatr = MINUS;
	mathomatic->n_trhs++;
	j = mathomatic->n_trhs;
	blt(&mathomatic->trhs[mathomatic->n_trhs], mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
	mathomatic->n_trhs += mathomatic->n_tlhs;
	for (; j < mathomatic->n_trhs; j++) {
		mathomatic->trhs[j].level += 2;
	}
	mathomatic->trhs[mathomatic->n_trhs].level = 1;
	mathomatic->trhs[mathomatic->n_trhs].kind = OPERATOR;
	mathomatic->trhs[mathomatic->n_trhs].token.operatr = DIVIDE;
	mathomatic->n_trhs++;
	mathomatic->trhs[mathomatic->n_trhs].level = 1;
	mathomatic->trhs[mathomatic->n_trhs].kind = CONSTANT;
	mathomatic->trhs[mathomatic->n_trhs].token.constant = iterations;
	mathomatic->n_trhs++;
	simp_loop(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
	dest[0] = mathomatic->zero_token;
	n1 = 1;
	for (j = 0; j <= iterations; j++) {
		if ((n1 + 1 + *nps) > mathomatic->n_tokens)
			error_huge(mathomatic);
		for (k = 0; k < n1; k++) {
			dest[k].level++;
		}
		ep = &dest[n1];
		ep->level = 1;
		ep->kind = OPERATOR;
		ep->token.operatr = PLUS;
		n1++;
		i1 = n1;
		blt(&dest[i1], source, *nps * sizeof(token_type));
		n1 += *nps;
		for (k = i1; k < n1; k++) {
			dest[k].level += 2;
		}
		for (k = i1; k < n1; k += 2) {
			if (dest[k].kind == VARIABLE && dest[k].token.variable == v) {
				level = dest[k].level;
				i2 = mathomatic->n_tlhs + 2 + mathomatic->n_trhs;
				if ((n1 + i2) > mathomatic->n_tokens)
					error_huge(mathomatic);
				blt(&dest[k+1+i2], &dest[k+1], (n1 - (k + 1)) * sizeof(token_type));
				n1 += i2;
				i2 = k;
				blt(&dest[k], mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
				k += mathomatic->n_tlhs;
				level++;
				for (; i2 < k; i2++) {
					dest[i2].level += level;
				}
				ep = &dest[k];
				ep->level = level;
				ep->kind = OPERATOR;
				ep->token.operatr = PLUS;
				ep++;
				level++;
				ep->level = level;
				ep->kind = CONSTANT;
				ep->token.constant = j;
				ep++;
				ep->level = level;
				ep->kind = OPERATOR;
				ep->token.operatr = TIMES;
				k += 3;
				i2 = k;
				blt(&dest[k], mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
				k += mathomatic->n_trhs;
				for (; i2 < k; i2++) {
					dest[i2].level += level;
				}
				k--;
			}
		}
		if (j > 0 && j < iterations) {
			if ((n1 + 2) > mathomatic->n_tokens)
				error_huge(mathomatic);
			ep = &dest[n1];
			ep->level = 2;
			ep->kind = OPERATOR;
			ep->token.operatr = TIMES;
			ep++;
			ep->level = 2;
			ep->kind = CONSTANT;
			if (trap_flag) {
				ep->token.constant = 2.0;
			} else {
				if ((j & 1) == 1) {
					ep->token.constant = 4.0;
				} else {
					ep->token.constant = 2.0;
				}
			}
			n1 += 2;
		}

		/* simplify and approximate the partial result quickly: */
		mathomatic->approximate_roots = true;
		elim_loop(mathomatic, dest, &n1);
		ufactor(mathomatic, dest, &n1);
		simp_divide(mathomatic, dest, &n1);
		factor_imaginary(mathomatic, dest, &n1);
		mathomatic->approximate_roots = false;
		side_debug(mathomatic, 1, dest, n1);

		if (exp_contains_infinity(dest, n1)) {
			error(mathomatic, _("Integration failed because result contains infinity or NaN (a singularity)."));
			return false;
		}
		/* detect an ever growing result: */
		switch (j) {
		case 0:
			break;
		case 1:
			first_size = n1;
			if (first_size < 4)
				first_size = 4;
			break;
		default:
			if ((n1 / 8) >= first_size) {
				error(mathomatic, _("Result growing, integration failed."));
				return false;
			}
			break;
		}
	}
	if ((n1 + 3 + mathomatic->n_trhs) > mathomatic->n_tokens)
		error_huge(mathomatic);
	for (k = 0; k < n1; k++)
		dest[k].level++;
	ep = &dest[n1];
	ep->level = 1;
	ep->kind = OPERATOR;
	ep->token.operatr = DIVIDE;
	ep++;
	ep->level = 1;
	ep->kind = CONSTANT;
	if (trap_flag) {
		ep->token.constant = 2.0;
	} else {
		ep->token.constant = 3.0;
	}
	ep++;
	ep->level = 1;
	ep->kind = OPERATOR;
	ep->token.operatr = TIMES;
	n1 += 3;
	k = n1;
	blt(&dest[k], mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
	n1 += mathomatic->n_trhs;
	for (; k < n1; k++)
		dest[k].level++;

	/* simplify and approximate the result even more: */
	mathomatic->approximate_roots = true;
	do {
		elim_loop(mathomatic, dest, &n1);
		ufactor(mathomatic, dest, &n1);
		simp_divide(mathomatic, dest, &n1);
	} while (factor_imaginary(mathomatic, dest, &n1));
	mathomatic->approximate_roots = false;

#if	!SILENT
	fprintf(mathomatic->gfp, _("Numerical integration successful:\n"));
#endif
	*np = n1;
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		blt(mathomatic->lhs[i], mathomatic->lhs[mathomatic->cur_equation], mathomatic->n_lhs[mathomatic->cur_equation] * sizeof(token_type));
		mathomatic->n_lhs[i] = mathomatic->n_lhs[mathomatic->cur_equation];
		if (solved && isvarchar(mathomatic, '\'')) {
			len = list_var(mathomatic, mathomatic->lhs[i][0].token.variable, 0);
			if (len > 0 && mathomatic->var_str[len-1] == '\'') {
				mathomatic->var_str[--len] = '\0';
			}
			parse_var(mathomatic, &mathomatic->lhs[i][0].token.variable, mathomatic->var_str);
		}
	}
	return return_result(mathomatic, i);
}
