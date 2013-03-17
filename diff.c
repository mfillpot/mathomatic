/*
 * Mathomatic symbolic differentiation routines and related commands.
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

static int d_recurse(token_type *equation, int *np, int loc, int level, long v);

/*
 * Compute the derivative of an equation side, with respect to variable "v",
 * using the fast, rule-based transform method.
 * This is done by recursively applying the proper rule of differentiation
 * for each operator encountered.
 *
 * Return true if successful.
 * The result must be simplified by the caller.
 */
int
differentiate(equation, np, v)
token_type	*equation;	/* pointer to source and destination equation side */
int		*np;		/* pointer to the length of the equation side */
long		v;		/* differentiation variable */
{
	int	i;

	organize(equation, np);
/* First put every times and divide on a level by itself. */
	for (i = 1; i < *np; i += 2) {
		switch (equation[i].token.operatr) {
		case TIMES:
		case DIVIDE:
			binary_parenthesize(equation, *np, i);
		}
	}
	return d_recurse(equation, np, 0, 1, v);
}

/*
 * Recursive differentiation routine.
 *
 * Symbolically differentiate expression in "equation"
 * (which is a standard equation side) starting at "loc".
 * The current level of parentheses is "level" and
 * do the differentiation with respect to variable "v".
 *
 * Return true if successful.
 * Return false if it is beyond this program's capabilities or an error was encountered.
 */
static int
d_recurse(equation, np, loc, level, v)
token_type	*equation;
int		*np, loc, level;
long		v;
{
	int		i, j;
	int		n;
	int		op;
	int		oploc, endloc;
	complexs	c;

	if (equation[loc].level < level) {
/* First differentiate if it is a single variable or constant. */
/* If it is the specified variable, change it to the constant 1, */
/* otherwise change it to the constant 0. */
		if (equation[loc].kind == VARIABLE
		    && ((v == MATCH_ANY && (equation[loc].token.variable & VAR_MASK) > SIGN)
		    || equation[loc].token.variable == v)) {
			equation[loc].kind = CONSTANT;
			equation[loc].token.constant = 1.0;
		} else {
			equation[loc].kind = CONSTANT;
			equation[loc].token.constant = 0.0;
		}
		return true;
	}
	for (op = 0, oploc = endloc = loc + 1; endloc < *np && equation[endloc].level >= level; endloc += 2) {
		if (equation[endloc].level == level) {
			switch (op) {
			case 0:
			case PLUS:
			case MINUS:
				break;
			default:
/* Oops.  More than one operator on the same level in this expression. */
				error_bug("Internal error in d_recurse(): differentiating with unparenthesized operators is not allowed.");
				return false;
			}
			op = equation[endloc].token.operatr;
			oploc = endloc;
		}
	}
	switch (op) {
	case 0:
	case PLUS:
	case MINUS:
		break;
	case TIMES:
		goto d_times;
	case DIVIDE:
		goto d_divide;
	case POWER:
		goto d_power;
	default:
/* Differentiate an unsupported operator. */
/* This is possible if the expression doesn't contain the specified variable. */
/* In that case, the expression is replaced with "0", otherwise return false. */
		for (i = loc; i < endloc; i += 2) {
			if (equation[i].kind == VARIABLE
			    && ((v == MATCH_ANY && (equation[i].token.variable & VAR_MASK) > SIGN)
			    || equation[i].token.variable == v)) {
				return false;
			}
		}
		blt(&equation[loc+1], &equation[endloc], (*np - endloc) * sizeof(token_type));
		*np -= (endloc - (loc + 1));
		equation[loc].level = level;
		equation[loc].kind = CONSTANT;
		equation[loc].token.constant = 0.0;
		return true;
	}
/* Differentiate PLUS and MINUS operators. */
/* Use addition rule: d(u+v) = d(u) + d(v), */
/* where "d()" is the derivative function */
/* and "u" and "v" are expressions. */
	for (i = loc; i < *np && equation[i].level >= level;) {
		if (equation[i].kind != OPERATOR) {
			if (!d_recurse(equation, np, i, level + 1, v))
				return false;
			i++;
			for (; i < *np && equation[i].level > level; i += 2)
				;
			continue;
		}
		i++;
	}
	return true;
d_times:
/* Differentiate TIMES operator. */
/* Use product rule: d(u*v) = u*d(v) + v*d(u). */
	if (*np + 1 + (endloc - loc) > n_tokens) {
		error_huge();
	}
	for (i = loc; i < endloc; i++)
		equation[i].level++;
	blt(&equation[endloc+1], &equation[loc], (*np - loc) * sizeof(token_type));
	*np += 1 + (endloc - loc);
	equation[endloc].level = level;
	equation[endloc].kind = OPERATOR;
	equation[endloc].token.operatr = PLUS;
	if (!d_recurse(equation, np, endloc + (oploc - loc) + 2, level + 2, v))
		return false;
	return(d_recurse(equation, np, loc, level + 2, v));
d_divide:
/* Differentiate DIVIDE operator. */
/* Use quotient rule: d(u/v) = (v*d(u) - u*d(v))/v^2. */
	if (*np + 3 + (endloc - loc) + (endloc - oploc) > n_tokens) {
		error_huge();
	}
	for (i = loc; i < endloc; i++)
		equation[i].level += 2;
	equation[oploc].token.operatr = TIMES;
	j = 1 + (endloc - loc);
	blt(&equation[endloc+1], &equation[loc], (*np - loc) * sizeof(token_type));
	*np += j;
	equation[endloc].level = level + 1;
	equation[endloc].kind = OPERATOR;
	equation[endloc].token.operatr = MINUS;
	j += endloc;
	blt(&equation[j+2+(endloc-oploc)], &equation[j], (*np - j) * sizeof(token_type));
	*np += 2 + (endloc - oploc);
	equation[j].level = level;
	equation[j].kind = OPERATOR;
	equation[j].token.operatr = DIVIDE;
	blt(&equation[j+1], &equation[oploc+1], (endloc - (oploc + 1)) * sizeof(token_type));
	j += endloc - oploc;
	equation[j].level = level + 1;
	equation[j].kind = OPERATOR;
	equation[j].token.operatr = POWER;
	j++;
	equation[j].level = level + 1;
	equation[j].kind = CONSTANT;
	equation[j].token.constant = 2.0;
	if (!d_recurse(equation, np, endloc + (oploc - loc) + 2, level + 3, v))
		return false;
	return(d_recurse(equation, np, loc, level + 3, v));
d_power:
/* Differentiate POWER operator. */
/* Since we don't have symbolic logarithms, do all we can without them. */
	for (i = oploc; i < endloc; i++) {
		if (equation[i].kind == VARIABLE
		    && ((v == MATCH_ANY && (equation[i].token.variable & VAR_MASK) > SIGN)
		    || equation[i].token.variable == v)) {
			if (parse_complex(&equation[loc], oploc - loc, &c)) {
				c = complex_log(c);
				n = (endloc - oploc) + 6;
				if (*np + n > n_tokens) {
					error_huge();
				}
				blt(&equation[endloc+n], &equation[endloc], (*np - endloc) * sizeof(token_type));
				*np += n;
				n = endloc;
				equation[n].level = level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
				equation[n].level = level + 1;
				equation[n].kind = CONSTANT;
				equation[n].token.constant = c.re;
				n++;
				equation[n].level = level + 1;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = PLUS;
				n++;
				equation[n].level = level + 2;
				equation[n].kind = CONSTANT;
				equation[n].token.constant = c.im;
				n++;
				equation[n].level = level + 2;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
				equation[n].level = level + 2;
				equation[n].kind = VARIABLE;
				equation[n].token.variable = IMAGINARY;
				n++;
				equation[n].level = level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
				blt(&equation[n], &equation[oploc+1], (endloc - (oploc + 1)) * sizeof(token_type));
				for (i = loc; i < endloc; i++) {
					equation[i].level++;
				}
				return(d_recurse(equation, np, n, level + 1, v));
			}
			return false;
		}
	}
	blt(scratch, &equation[oploc+1], (endloc - (oploc + 1)) * sizeof(token_type));
	n = endloc - (oploc + 1);
	scratch[n].level = level;
	scratch[n].kind = OPERATOR;
	scratch[n].token.operatr = TIMES;
	n++;
	if (n + (endloc - loc) + 2 > n_tokens) {
		error_huge();
	}
	blt(&scratch[n], &equation[loc], (endloc - loc) * sizeof(token_type));
	i = n;
	n += oploc + 1 - loc;
	for (; i < n; i++)
		scratch[i].level++;
	n += endloc - (oploc + 1);
	for (; i < n; i++)
		scratch[i].level += 2;
	scratch[n].level = level + 2;
	scratch[n].kind = OPERATOR;
	scratch[n].token.operatr = MINUS;
	n++;
	scratch[n].level = level + 2;
	scratch[n].kind = CONSTANT;
	scratch[n].token.constant = 1.0;
	n++;
	if (n + (oploc - loc) + 1 > n_tokens) {
		error_huge();
	}
	scratch[n].level = level;
	scratch[n].kind = OPERATOR;
	scratch[n].token.operatr = TIMES;
	n++;
	j = n;
	blt(&scratch[n], &equation[loc], (oploc - loc) * sizeof(token_type));
	n += oploc - loc;
	if (*np - (endloc - loc) + n > n_tokens) {
		error_huge();
	}
	blt(&equation[loc+n], &equation[endloc], (*np - endloc) * sizeof(token_type));
	*np += loc + n - endloc;
	blt(&equation[loc], scratch, n * sizeof(token_type));
	return(d_recurse(equation, np, loc + j, level + 1, v));
}

/*
 * The derivative command.
 */
int
derivative_cmd(cp)
char	*cp;
{
	int		i, len;
	long		v = 0;		/* Mathomatic variable */
	long		l1, order = 1;
	token_type	*source, *dest;
	int		n1, *nps, *np;
	int		simplify_flag = true, solved;

	if (current_not_defined()) {
		return false;
	}
	solved = solved_equation(cur_equation);
	if (strcmp_tospace(cp, "nosimplify") == 0) {
		simplify_flag = false;
		cp = skip_param(cp);
	}
	i = next_espace();
	if (n_rhs[cur_equation]) {
		if (!solved) {
			warning(_("Not a solved equation.  Only the RHS will be differentiated."));
		}
		source = rhs[cur_equation];
		nps = &n_rhs[cur_equation];
		dest = rhs[i];
		np = &n_rhs[i];
	} else {
		source = lhs[cur_equation];
		nps = &n_lhs[cur_equation];
		dest = lhs[i];
		np = &n_lhs[i];
	}
/* parse the command line or prompt: */
	if (*cp) {
		if (is_all(cp)) {
			cp = skip_param(cp);
			v = MATCH_ANY;
		} else {
			if (isvarchar(*cp)) {
				cp = parse_var2(&v, cp);
				if (cp == NULL) {
					return false;
				}
			}
		}
		if (*cp) {
			order = decstrtol(cp, &cp);
		}
		if (order <= 0) {
			error(_("The order must be a positive integer."));
			return false;
		}
		if (extra_characters(cp))
			return false;
	}
	if (no_vars(source, *nps, &v)) {
		warning(_("Current expression contains no variables; the derivative will be zero."));
	} else {
		if (v == 0) {
			if (!prompt_var(&v)) {
				return false;
			}
		}
		if (v && v != MATCH_ANY && !found_var(source, *nps, v)) {
			warning(_("Specified variable not found; the derivative will be zero."));
		}
	}
	if (v == 0) {
		error(_("No differentiation variable specified."));
		return false;
	}
#if	!SILENT
	list_var(v, 0);
	if (n_rhs[cur_equation]) {
		fprintf(gfp, _("Differentiating the RHS with respect to %s"), var_str);
	} else {
		fprintf(gfp, _("Differentiating with respect to %s"), var_str);
	}
	if (order != 1) {
		fprintf(gfp, _(" %ld times"), order);
	}
	if (simplify_flag) {
		fprintf(gfp, _(" and simplifying"));
	} else {
		fprintf(gfp, _(" and not simplifying"));
	}
	fprintf(gfp, "...\n");
#endif
	blt(dest, source, *nps * sizeof(token_type));
	n1 = *nps;
/* do the actual differentiating and simplifying: */
	for (l1 = 0; l1 < order; l1++) {
		if (order != 1) {
			if (n1 == 1 && dest[0].kind == CONSTANT && dest[0].token.constant == 0.0) {
				fprintf(gfp, _("0 reached after %ld derivatives taken.\n"), l1);
				order = l1;
				break;
			}
		}
		if (!differentiate(dest, &n1, v)) {
			error(_("Differentiation failed."));
			return false;
		}
		if (simplify_flag) {
			simpa_repeat_side(dest, &n1, true, false);
		} else {
			elim_loop(dest, &n1);
		}
	}
	*np = n1;
	if (n_rhs[cur_equation]) {
		blt(lhs[i], lhs[cur_equation], n_lhs[cur_equation] * sizeof(token_type));
		n_lhs[i] = n_lhs[cur_equation];
		if (solved && isvarchar('\'')) {
			len = list_var(lhs[i][0].token.variable, 0);
			for (l1 = 0; l1 < order && len < (MAX_VAR_LEN - 1); l1++) {
				var_str[len++] = '\'';
			}
			var_str[len] = '\0';
			if (l1 == order) {
				parse_var(&lhs[i][0].token.variable, var_str);
			}
		}
	}
	cur_equation = i;
	return return_result(cur_equation);
}

/*
 * The extrema command.
 */
int
extrema_cmd(cp)
char	*cp;
{
	int		i;
	long		v = 0;		/* Mathomatic variable */
	long		l1, order = 1;
	token_type	want;
	token_type	*source;
	int		n;

	if (current_not_defined()) {
		return false;
	}
	i = next_espace();
	if (n_rhs[cur_equation]) {
		if (!solved_equation(cur_equation)) {
			error(_("The current equation is not solved for a variable."));
			return false;
		}
		source = rhs[cur_equation];
		n = n_rhs[cur_equation];
	} else {
		source = lhs[cur_equation];
		n = n_lhs[cur_equation];
	}
	if (*cp) {
		if (isvarchar(*cp)) {
			cp = parse_var2(&v, cp);
			if (cp == NULL) {
				return false;
			}
		}
		if (*cp) {
			order = decstrtol(cp, &cp);
		}
		if (order <= 0) {
			error(_("The order must be a positive integer."));
			return false;
		}
		if (extra_characters(cp))
			return false;
	}
	show_usage = false;
	if (no_vars(source, n, &v)) {
		error(_("Current expression contains no variables."));
		return false;
	}
	if (v == 0) {
		if (!prompt_var(&v)) {
			return false;
		}
	}
	if (!found_var(source, n, v)) {
		error(_("Specified variable not found; the derivative would be zero."));
		return false;
	}
	blt(rhs[i], source, n * sizeof(token_type));
/* take derivatives with respect to the specified variable and simplify: */
	for (l1 = 0; l1 < order; l1++) {
		if (!differentiate(rhs[i], &n, v)) {
			error(_("Differentiation failed."));
			return false;
		}
		simpa_repeat_side(rhs[i], &n, true, false);
	}
	if (!found_var(rhs[i], n, v)) {
		error(_("There are no solutions."));
		return false;
	}
	n_rhs[i] = n;
/* set equal to zero: */
	n_lhs[i] = 1;
	lhs[i][0] = zero_token;
	cur_equation = i;
/* lastly, solve for the specified variable and simplify: */
	want.level = 1;
	want.kind = VARIABLE;
	want.token.variable = v;
	if (solve_sub(&want, 1, lhs[i], &n_lhs[i], rhs[i], &n_rhs[i]) <= 0) {
		error(_("Solve failed."));
		return false;
	}
	simpa_repeat_side(rhs[i], &n_rhs[i], false, false);
	return return_result(cur_equation);
}

/*
 * The taylor command.
 */
int
taylor_cmd(cp)
char	*cp;
{
	long		v = 0;			/* Mathomatic variable */
	int		i, j, k, i1;
	int		level;
	long		l1, n, order = -1L;
	double		d;
	char		*cp_start, *cp1 = NULL, buf[MAX_CMD_LEN];
	int		our;
	int		our_nlhs, our_nrhs;
	token_type	*ep, *source, *dest;
	int		n1, *nps, *np;
	int		simplify_flag = true;

	cp_start = cp;
	if (current_not_defined()) {
		return false;
	}
	if (strcmp_tospace(cp, "nosimplify") == 0) {
		simplify_flag = false;
		cp = skip_param(cp);
	}
	i = next_espace();
	blt(lhs[i], lhs[cur_equation], n_lhs[cur_equation] * sizeof(token_type));
	n_lhs[i] = n_lhs[cur_equation];
	n_rhs[i] = 0;
        our = alloc_next_espace();
	n_lhs[i] = 0;
        if (our < 0) {
                error(_("Out of free equation spaces."));
		show_usage = false;
		return false;
	}
	if (n_rhs[cur_equation]) {
		source = rhs[cur_equation];
		nps = &n_rhs[cur_equation];
		dest = rhs[i];
		np = &n_rhs[i];
	} else {
		source = lhs[cur_equation];
		nps = &n_lhs[cur_equation];
		dest = lhs[i];
		np = &n_lhs[i];
	}
	if (*cp && isvarchar(*cp)) {
		cp = parse_var2(&v, cp);
		if (cp == NULL) {
			return false;
		}
	}
	if (*cp) {
		order = decstrtol(cp, &cp1);
		if (cp1 != skip_param(cp) || order < 0) {
			error(_("Positive integer required for order."));
			return false;
		}
		cp = cp1;
	}
	show_usage = false;
	no_vars(source, *nps, &v);
	if (v == 0) {
		if (!prompt_var(&v)) {
			return false;
		}
	}
	if (!found_var(source, *nps, v)) {
		warning(_("Specified differentiation variable not found; the derivative will be 0."));
	}
	blt(rhs[our], source, *nps * sizeof(token_type));
	our_nrhs = *nps;
/* Simplify and take the first derivative: */
	uf_simp(rhs[our], &our_nrhs);
	if (!differentiate(rhs[our], &our_nrhs, v)) {
		error(_("Differentiation failed."));
		return false;
	}
	if (*cp) {
		input_column += (cp - cp_start);
		cp = parse_expr(lhs[our], &our_nlhs, cp, true);
		if (cp == NULL || extra_characters(cp) || our_nlhs <= 0) {
			show_usage = true;
			return false;
		}
	} else {
#if	!SILENT
		list_var(v, 0);
		printf(_("Taylor series expansion around %s = point.\n"), var_str);
#endif
		my_strlcpy(prompt_str, _("Enter point (an expression; usually 0): "), sizeof(prompt_str));
		if (!get_expr(lhs[our], &our_nlhs)) {
			return false;
		}
	}
	if (order < 0) {
		my_strlcpy(prompt_str, _("Enter order (number of derivatives to take): "), sizeof(prompt_str));
		if ((cp1 = get_string(buf, sizeof(buf))) == NULL)
			return false;
		if (*cp1) {
			cp = NULL;
			order = decstrtol(cp1, &cp);
			if (cp == NULL || *cp || order < 0) {
				error(_("Positive integer required for order."));
				return false;
			}
		} else {
			order = LONG_MAX - 1;
#if	!SILENT
			printf(_("Derivatives will be taken until they reach zero...\n"));
#endif
		}
	}
#if	!SILENT
	fprintf(gfp, _("Taylor series"));
	if (n_rhs[cur_equation]) {
		fprintf(gfp, _(" of the RHS"));
	}
	list_var(v, 0);
	fprintf(gfp, _(" with respect to %s"), var_str);
	if (simplify_flag) {
		fprintf(gfp, _(", simplified"));
	} else {
		fprintf(gfp, _(", not simplified"));
	}
	fprintf(gfp, "...\n");
#endif
	n = 0;
	i1 = 0;
	blt(dest, source, *nps * sizeof(token_type));
	n1 = *nps;
loop_again:
	for (k = i1; k < n1; k += 2) {
		if (dest[k].kind == VARIABLE && dest[k].token.variable == v) {
			level = dest[k].level;
			if ((n1 + our_nlhs - 1) > n_tokens)
				error_huge();
			blt(&dest[k+our_nlhs], &dest[k+1], (n1 - (k + 1)) * sizeof(token_type));
			n1 += our_nlhs - 1;
			j = k;
			blt(&dest[k], lhs[our], our_nlhs * sizeof(token_type));
			k += our_nlhs;
			for (; j < k; j++)
				dest[j].level += level;
			k--;
		}
	}
	if ((n1 + our_nlhs + 7) > n_tokens)
		error_huge();
	for (k = i1; k < n1; k++)
		dest[k].level++;
	ep = &dest[n1];
	ep->level = 1;
	ep->kind = OPERATOR;
	ep->token.operatr = TIMES;
	ep++;
	ep->level = 3;
	ep->kind = VARIABLE;
	ep->token.variable = v;
	ep++;
	ep->level = 3;
	ep->kind = OPERATOR;
	ep->token.operatr = MINUS;
	n1 += 3;
	j = n1;
	blt(&dest[n1], lhs[our], our_nlhs * sizeof(token_type));
	n1 += our_nlhs;
	for (; j < n1; j++)
		dest[j].level += 3;
	ep = &dest[n1];
	ep->level = 2;
	ep->kind = OPERATOR;
	ep->token.operatr = POWER;
	ep++;
	ep->level = 2;
	ep->kind = CONSTANT;
	ep->token.constant = n;
	ep++;
	ep->level = 1;
	ep->kind = OPERATOR;
	ep->token.operatr = DIVIDE;
	ep++;
	for (d = 1.0, l1 = 2; l1 <= n; l1++)
		d *= l1;
	ep->level = 1;
	ep->kind = CONSTANT;
	ep->token.constant = d;
	n1 += 4;
	for (; i1 < n1; i1++)
		dest[i1].level++;
	if (simplify_flag) {
		uf_simp(dest, &n1);
	}
	side_debug(1, dest, n1);
	if (exp_contains_infinity(dest, n1)) {
		error(_("Result invalid because it contains infinity or NaN."));
		return false;
	}
	if (n < order) {
		if (n > 0) {
			if (!differentiate(rhs[our], &our_nrhs, v)) {
				error(_("Differentiation failed."));
				return false;
			}
		}
/*		symb_flag = symblify; */
		simpa_repeat_side(rhs[our], &our_nrhs, true, false /* was true */);
/*		symb_flag = false; */
		if (our_nrhs != 1 || rhs[our][0].kind != CONSTANT || rhs[our][0].token.constant != 0.0) {
			i1 = n1;
			if ((i1 + 1 + our_nrhs) > n_tokens)
				error_huge();
			for (j = 0; j < i1; j++)
				dest[j].level++;
			dest[i1].level = 1;
			dest[i1].kind = OPERATOR;
			dest[i1].token.operatr = PLUS;
			i1++;
			blt(&dest[i1], rhs[our], our_nrhs * sizeof(token_type));
			n1 = i1 + our_nrhs;
			n++;
			goto loop_again;
		}
	}
#if	!SILENT
	fprintf(gfp, _("%ld non-zero derivative%s applied.\n"), n, (n == 1) ? "" : "s");
#endif
	if (n_rhs[cur_equation]) {
		n_lhs[i] = n_lhs[cur_equation];
	}
	*np = n1;
	cur_equation = i;
	return return_result(cur_equation);
}

/*
 * The limit command.
 */
int
limit_cmd(cp)
char	*cp;
{
	int		i;
	long		v = 0;			/* Mathomatic variable */
	token_type	solved_v, want;
	char		*cp_start;

	cp_start = cp;
	if (current_not_defined()) {
		return false;
	}
	i = next_espace();
	if (n_rhs[cur_equation] == 0) {
/* make expression into an equation: */
		blt(rhs[cur_equation], lhs[cur_equation], n_lhs[cur_equation] * sizeof(token_type));
		n_rhs[cur_equation] = n_lhs[cur_equation];
		n_lhs[cur_equation] = 1;
		lhs[cur_equation][0].level = 1;
		lhs[cur_equation][0].kind = VARIABLE;
		parse_var(&lhs[cur_equation][0].token.variable, "limit");
	}
	if (!solved_equation(cur_equation)) {
		error(_("The current equation is not solved for a variable."));
		return false;
	}
	solved_v = lhs[cur_equation][0];
/* parse the command line or prompt: */
	if (*cp) {
		cp = parse_var2(&v, cp);
		if (cp == NULL) {
			return false;
		}
	}
	show_usage = false;
	if (no_vars(rhs[cur_equation], n_rhs[cur_equation], &v)) {
		warning(_("Current expression contains no variables; that is the answer."));
		return return_result(cur_equation);
	}
	if (v == 0) {
		if (!prompt_var(&v)) {
			return false;
		}
	}
	if (!found_var(rhs[cur_equation], n_rhs[cur_equation], v)) {
		warning(_("Limit variable not found; answer is original expression."));
		return return_result(cur_equation);
	}
	if (*cp == '=') {
		cp = skip_space(cp + 1);
	}
	if (*cp) {
		input_column += (cp - cp_start);
		cp = parse_expr(tes, &n_tes, cp, true);
		if (cp == NULL || extra_characters(cp) || n_tes <= 0) {
			show_usage = true;
			return false;
		}
	} else {
		list_var(v, 0);
		snprintf(prompt_str, sizeof(prompt_str), _("as %s goes to: "), var_str);
		if (!get_expr(tes, &n_tes)) {
			return false;
		}
	}
	simp_loop(tes, &n_tes);
#if	!SILENT
	list_var(v, 0);
	fprintf(gfp, _("Taking the limit as %s goes to "), var_str);
	list_proc(tes, n_tes, false);
	fprintf(gfp, "\n");
#endif
/* copy the current equation to a new equation space, then simplify and work on the copy: */
	copy_espace(cur_equation, i);
	simpa_side(rhs[i], &n_rhs[i], false, false);

/* see if the limit expression is positive infinity: */
	if (n_tes == 1 && tes[0].kind == CONSTANT && tes[0].token.constant == INFINITY) {
/* To take the limit to positive infinity, */
/* replace infinity with zero and replace the limit variable with its reciprocal: */
		n_tes = 1;
		tes[0] = zero_token;
		tlhs[0] = one_token;
		tlhs[1].level = 1;
		tlhs[1].kind = OPERATOR;
		tlhs[1].token.operatr = DIVIDE;
		tlhs[2].level = 1;
		tlhs[2].kind = VARIABLE;
		tlhs[2].token.variable = v;
		n_tlhs = 3;
		subst_var_with_exp(rhs[i], &n_rhs[i], tlhs, n_tlhs, v);
	}

/* General limit taking, solve for the limit variable: */
	debug_string(0, _("Solving..."));
	want.level = 1;
	want.kind = VARIABLE;
	want.token.variable = v;
	if (solve_sub(&want, 1, lhs[i], &n_lhs[i], rhs[i], &n_rhs[i]) <= 0) {
		error(_("Can't take the limit because solve failed."));
		return false;
	}
/* replace the limit variable (LHS) with the limit expression: */
	blt(lhs[i], tes, n_tes * sizeof(token_type));
	n_lhs[i] = n_tes;
/* simplify the RHS: */
	symb_flag = symblify;
	simpa_side(rhs[i], &n_rhs[i], false, false);
	symb_flag = false;
	if (exp_contains_nan(rhs[i], n_rhs[i])) {
		error(_("Unable to take limit; result contains NaN (Not a Number)."));
		return false;
	}
/* solve back for the original variable: */
	if (solve_sub(&solved_v, 1, lhs[i], &n_lhs[i], rhs[i], &n_rhs[i]) <= 0) {
		error(_("Can't take the limit because solve failed."));
		return false;
	}
/* simplify before returning the result: */
	simpa_side(rhs[i], &n_rhs[i], false, false);
	if (exp_contains_nan(rhs[i], n_rhs[i])) {
		error(_("Unable to take limit; result contains NaN (Not a Number)."));
		return false;
	}
	return return_result(i);
}
