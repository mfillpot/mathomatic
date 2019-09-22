/*
 * Expression parsing routines for Mathomatic.
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
 * Convert all alphabetic characters in a string to lower case.
 */
void
str_tolower(cp)
char	*cp;
{
	if (cp) {
		for (; *cp; cp++) {
			if (isascii(*cp) && isupper(*cp))
				*cp = tolower(*cp);
		}
	}
}

/*
 * Display an up arrow pointing to the error, if appropriate.
 * The up arrow is positioned under the input string,
 * followed by the error message, which must be a constant string.
 */
void
put_up_arrow(MathoMatic* mathomatic, int cnt, char *cp)
//int	cnt;	/* position of error, relative to "input_column" */
//char	*cp;	/* error message (constant ASCII string) */
{
#if	!SILENT && !LIBRARY
	int	 i;

	cnt += mathomatic->input_column;
	if (!mathomatic->quiet_mode && mathomatic->point_flag && (mathomatic->screen_columns == 0 || cnt < mathomatic->screen_columns)) {
		for (i = 0; i < cnt; i++) {
			printf(" ");
		}
		printf("^ ");
	}
#endif
	error(mathomatic, cp);
}

/*
 * Return true if character is a valid starting variable character.
 */
int
isvarchar(MathoMatic* mathomatic, int ch)
{
	if (isdigit(ch)) {	/* variable names can never start with a digit */
		return false;
	}
	return(ch == '_' || (ch && strchr(mathomatic->special_variable_characters, ch)) || isalpha(ch));
}

/*
 * Return +1 for an opening parenthesis, -1 for a closing parenthesis.
 * Otherwise, return 0.
 */
int
paren_increment(ch)
int	ch;
{
	switch (ch) {
	case '(':
		return 1;
	case ')':
		return -1;
	}
	return 0;
}

/*
 * Return true if character (ch) is the beginning of a Mathomatic operator.
 */
int
is_mathomatic_operator(ch)
int	ch;
{
	switch (ch) {
	case '!':
	case '*':
	case '^':
	case '/':
	case '%':
	case '+':
	case '-':
	case '=':
	case '|':
		return true;
	}
	return false;
}

/*
 * Parenthesize an operator.
 */
void
binary_parenthesize(p1, n, i)
token_type	*p1;	/* pointer to expression */
int		n;	/* length of expression */
int		i;	/* location of operator to parenthesize in expression */
{
	int	j;
	int	level;
	int	skip_negate;

#if	DEBUG
	if (i >= (n - 1) || (n & 1) != 1 || (i & 1) != 1 || p1[i].kind != OPERATOR) {
		error_bug(mathomatic, "Internal error in arguments to binary_parenthesize().");
	}
#endif
	skip_negate = (p1[i].token.operatr != NEGATE);
	level = p1[i].level++;
	if (p1[i-1].level++ > level) {
		for (j = i - 2; j >= 0; j--) {
			if (p1[j].level <= level)
				break;
			p1[j].level++;
		}
	}
finish:
	if (p1[i+1].level++ > level) {
		for (j = i + 2; j < n; j++) {
			if (p1[j].level <= level)
				break;
			p1[j].level++;
		}
	} else if (skip_negate && p1[i+1].level > level && i + 3 < n && p1[i+2].level == level
	    && p1[i+2].token.operatr == NEGATE) {
		p1[i+2].level++;
		i += 2;
		goto finish;
	}
}

/*
 * Handle and remove the special NEGATE operator.
 */
void
handle_negate(equation, np)
token_type	*equation;
int		*np;
{
	int	i;

	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr == NEGATE) {
			/* parenthesize it first: */
			binary_parenthesize(equation, *np, i);
			/* finish changing negate operator to -1.0 times the operand: */
			equation[i].token.operatr = TIMES;
		}
	}
}

/*
 * Parenthesize most operators so that expression evaluation is in the correct order.
 * Gives different operators on the same level in an expression the correct priority.
 * Similar operators on the same level are always evaluated or grouped left to right,
 * except for the power operator.
 *
 * organize() should be called after this to remove unneeded parentheses.
 */
void
give_priority(MathoMatic* mathomatic, token_type *equation, int *np)
//token_type	*equation;	/* pointer to expression */
//int		*np;		/* pointer to expression length */
{
	int	i;

/* Higher priority (precedence) operators need to be parenthesized first: */
	for (i = 1; i < *np; i += 2) {
		if (equation[i].token.operatr >= FACTORIAL) {
			binary_parenthesize(equation, *np, i);
		}
	}

	if (mathomatic->right_associative_power) {
		for (i = *np - 2; i >= 1; i -= 2) {	/* count down (for right to left evaluation) */
			if (equation[i].token.operatr == POWER) {
				binary_parenthesize(equation, *np, i);
			}
		}
	} else {
		for (i = 1; i < *np; i += 2) {		/* count up (for left to right evaluation) */
			if (equation[i].token.operatr == POWER) {
				binary_parenthesize(equation, *np, i);
			}
		}
	}

	for (i = 1; i < *np; i += 2) {
		switch (equation[i].token.operatr) {
		case TIMES:
		case DIVIDE:
		case MODULUS:
		case IDIVIDE:
			binary_parenthesize(equation, *np, i);
			break;
		}
	}
	handle_negate(equation, np);	/* Make this the first called function here to make negate highest priority. */
}

/*
 * This is a simple, non-recursive mathematical expression parser.
 * To parse, the character string is sequentially read and stored
 * in the internal storage format.
 * The maximum length that can be parsed is the size of an equation side (n_tokens).
 * Any syntax and other errors are carefully reported and give a NULL return.
 *
 * Returns the new string position, or NULL if error.
 */
char *
parse_section(MathoMatic* mathomatic, token_type *equation, int *np, char *cp, int allow_space)
//token_type	*equation;	/* where the parsed expression is stored (equation side) */
//int		*np;		/* pointer to the returned parsed expression length */
//char		*cp;		/* string to parse */
//int		allow_space;	/* if false, any space characters terminate parsing */
{
	int		i;
	int		n = 0, old_n;		/* position in equation[] */
	int		cur_level = 1;		/* current level of parentheses */
	int		operand = false;	/* flip-flop between operand and operator */
	char		*cp_start, *cp1;
	double		d;
	int		abs_count = 0;
	int		abs_array[10];

	if (cp == NULL)
		return(NULL);
	cp_start = cp;
	for (;; cp++) {
		if (n > (mathomatic->n_tokens - 10)) {
			error_huge(mathomatic);
		}
		switch (*cp) {
		case '(':
		case '{':
			if (operand) {
#if	1
/* Allow things like (x)(y) and x{y} by defaulting to the times operator.  The result is x*y. */
				operand = false;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
#else
				goto syntax_error;
#endif
			}
			cur_level++;
			continue;
		case ')':
		case '}':
			cur_level--;
			if (cur_level <= 0 || (abs_count > 0 && cur_level < abs_array[abs_count-1])) {
				put_up_arrow(mathomatic, cp - cp_start, _("Unmatched parenthesis: too many )"));
				return(NULL);
			}
			if (!operand) {
				goto syntax_error;
			}
			continue;
		case ' ':
		case '\t':
		case '\f':
			if (allow_space)
				continue;
		case ',':
		case '=':
		case ';':
		case '\0':
		case '\n':
			goto p_out;	/* terminator encountered */
		case '\r':
			continue;
		case '\033':
			if (cp[1] == '[' || cp[1] == 'O') {
				error(mathomatic, _("Cursor or function key string encountered, unable to interpret."));
				return(NULL);
			}
			continue;
		}
		operand = !operand;
		switch (*cp) {
		case '|':
			if (operand) {
				if (abs_count >= ARR_CNT(abs_array)) {
					error(mathomatic, _("Too many nested absolute values."));
					return(NULL);
				}
				cur_level += 3;
				abs_array[abs_count++] = cur_level;
			} else {
				if (abs_count <= 0 || cur_level != abs_array[--abs_count]) {
					goto syntax_error;
				}
				cur_level--;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = POWER;
				n++;
				equation[n].level = cur_level;
				equation[n].kind = CONSTANT;
				equation[n].token.constant = 2.0;
				n++;
				cur_level--;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = POWER;
				n++;
				equation[n].level = cur_level;
				equation[n].kind = CONSTANT;
				equation[n].token.constant = 0.5;
				n++;
				cur_level--;
			}
			operand = !operand;
			break;
		case '!':
			if (operand) {
				goto syntax_error;
			}
			if (cp[1] == '!' && cp[2] != '!') {
				warning(mathomatic, _("Multifactorial not implemented, using x!! = (x!)!"));
			}
			equation[n].level = cur_level;
			equation[n].kind = OPERATOR;
			equation[n].token.operatr = FACTORIAL;
			n++;
			equation[n].level = cur_level;
			equation[n].kind = CONSTANT;
			equation[n].token.constant = 1.0;
			n++;
			operand = true;
			break;
		case '^':
parse_power:
			if (operand) {
				goto syntax_error;
			}
			equation[n].level = cur_level;
			equation[n].kind = OPERATOR;
			equation[n].token.operatr = POWER;
			n++;
			break;
		case '*':
			if (cp[1] == '*') {	/* for "**" */
				cp++;
				goto parse_power;
			}
			if (operand) {
				goto syntax_error;
			}
			equation[n].level = cur_level;
			equation[n].kind = OPERATOR;
			equation[n].token.operatr = TIMES;
			n++;
			break;
		case '/':
			if (operand) {
				goto syntax_error;
			}
			if (cp[1] == '/') {
				cp++;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = IDIVIDE;
			} else {
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = DIVIDE;
			}
			n++;
			break;
		case '%':
			if (operand) {
#if	1
/* Allow Maxima's %e, %i, and %pi by ignoring % in the wrong place. */
				if (isalpha(cp[1])) {
					operand = false;
					break;
				}
#endif
				goto syntax_error;
			}
			equation[n].level = cur_level;
			equation[n].kind = OPERATOR;
			equation[n].token.operatr = MODULUS;
			n++;
			break;
		case '+':
		case '-':
			if (!operand) {
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = ((*cp == '+') ? PLUS : MINUS);
				n++;
			}
			if (strncmp(cp, "+/-", 3) == 0) {
				equation[n].level = cur_level;
				equation[n].kind = VARIABLE;
				next_sign(mathomatic, &equation[n].token.variable);
				n++;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
				cp += 2;
				operand = false;
				break;
			}
			if (!operand) {
				break;
			}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
			if (!operand) {
				operand = true;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
			}
			if (*cp == '-') {
				equation[n].kind = CONSTANT;
				equation[n].token.constant = -1.0;
				equation[n].level = cur_level;
				n++;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = NEGATE;
				equation[n].level = cur_level;
				n++;
				operand = false;
				continue;
			}
			cp1 = cp;
			errno = 0;
			d = strtod(cp1, &cp);
			if (cp == cp1) {
				goto syntax_error;
			}
			if (errno) {
				put_up_arrow(mathomatic, cp1 - cp_start, _("Constant out of range."));
				return(NULL);
			}
			equation[n].kind = CONSTANT;
			equation[n].token.constant = d;
			equation[n].level = cur_level;
			n++;
			cp--;
			break;
		case '#':
			if (!operand) {
				goto syntax_error;
			}
			cp++;
			cp1 = NULL;
			switch (*cp) {
			case '+':
			case '-':
				i = strtol(cp, &cp1, 10);
				i = mathomatic->cur_equation + i;
				break;
			default:
				i = strtol(cp, &cp1, 10) - 1;
				break;
			}
			if (cp1 == NULL || cp == cp1) {
				put_up_arrow(mathomatic, cp - cp_start, _("Error parsing equation space number after #."));
				return NULL;
			}
			if (empty_equation_space(mathomatic, i)) {
				put_up_arrow(mathomatic, cp - cp_start, _("No expression available in # specified equation space."));
				return NULL;
			}
			cp = cp1 - 1;
			old_n = n;
			if (mathomatic->n_rhs[i]) {
				n += mathomatic->n_rhs[i];
				if (n > mathomatic->n_tokens) {
					error_huge(mathomatic);
				}
				blt(&equation[old_n], mathomatic->rhs[i], mathomatic->n_rhs[i] * sizeof(token_type));
			} else {
				n += mathomatic->n_lhs[i];
				if (n > mathomatic->n_tokens) {
					error_huge(mathomatic);
				}
				blt(&equation[old_n], mathomatic->lhs[i], mathomatic->n_lhs[i] * sizeof(token_type));
			}
			for (; old_n < n; old_n++) {
				equation[old_n].level += cur_level;
			}
			break;
		default:
			if (!isvarchar(mathomatic, *cp)) {
				put_up_arrow(mathomatic, cp - cp_start, _("Unrecognized character."));
				return(NULL);
			}
			if (!operand) {
				operand = true;
				equation[n].level = cur_level;
				equation[n].kind = OPERATOR;
				equation[n].token.operatr = TIMES;
				n++;
			}
			cp1 = cp;
			if (strncasecmp(cp, "inf", strlen("inf")) == 0
			    && !isvarchar(mathomatic, cp[strlen("inf")])) {
				equation[n].kind = CONSTANT;
				equation[n].token.constant = INFINITY;	/* the infinity constant */
				cp += strlen("inf");
			} else if (strncasecmp(cp, INFINITY_NAME, strlen(INFINITY_NAME)) == 0
			    && !isvarchar(mathomatic, cp[strlen(INFINITY_NAME)])) {
				equation[n].kind = CONSTANT;
				equation[n].token.constant = INFINITY;	/* the infinity constant */
				cp += strlen(INFINITY_NAME);
			} else {
				equation[n].kind = VARIABLE;
				cp = parse_var(mathomatic, &equation[n].token.variable, cp);
				if (cp == NULL) {
					return(NULL);
				}
			}
			if (*cp == '(') {
/* Named functions currently not implemented, except when using m4. */
#if	LIBRARY
				put_up_arrow(mathomatic, cp1 - cp_start, _("Unknown function."));
#else
				put_up_arrow(mathomatic, cp1 - cp_start, _("Unknown function; try using rmath, which allows basic functions."));
#endif
				return(NULL);
			}
			cp--;
			equation[n].level = cur_level;
			n++;
			break;
		}
	}
p_out:
	if (abs_count != 0 || (n && !operand)) {
		goto syntax_error;
	}
	if (cur_level != 1) {
		put_up_arrow(mathomatic, cp - cp_start, _("Unmatched parenthesis: missing )"));
		return(NULL);
	}
	while (*cp == '=')
		cp++;
	*np = n;
	if (n) {
		give_priority(mathomatic, equation, np);
		organize(mathomatic, equation, np);
	}
	mathomatic->input_column += (cp - cp_start);
	return cp;

syntax_error:
	put_up_arrow(mathomatic, cp - cp_start, _("Syntax error."));
	return(NULL);
}

/*
 * Parse an equation string into equation space "n".
 * The result will not necessarily be an equation,
 * because this can parse any expression or equation.
 *
 * Returns the new string position, or NULL if error.
 * Currently, there can be no more to parse in the string when this returns.
 */
char *
parse_equation(MathoMatic* mathomatic, int n, char *cp)
//int	n;	/* equation space number */
//char	*cp;	/* pointer to the beginning of the equation character string */
{
	if ((cp = parse_expr(mathomatic, mathomatic->lhs[n], &mathomatic->n_lhs[n], cp, true)) != NULL) {
		if ((cp = parse_expr(mathomatic, mathomatic->rhs[n], &mathomatic->n_rhs[n], cp, true)) != NULL) {
			if (!extra_characters(mathomatic, cp))
				return cp;
		}
	}
	mathomatic->n_lhs[n] = 0;
	mathomatic->n_rhs[n] = 0;
	return NULL;
}

/*
 * Parse an expression (not an equation) string, with equation space pull
 * if the string contains "#" followed by an equation number.
 *
 * Returns the new string position, or NULL if error.
 */
char *
parse_expr(MathoMatic* mathomatic, token_type *equation, int *np, char *cp, int allow_space)
//token_type	*equation;	/* where the parsed expression is stored (equation side) */
//int		*np;		/* pointer to the returned parsed expression length */
//char		*cp;		/* string to parse */
//int		allow_space;	/* if true, allow and ignore space characters; if false, space means terminate parsing */
{
	if (cp == NULL)
		return NULL;
	if (!mathomatic->case_sensitive_flag) {
		str_tolower(cp);
	}
	cp = parse_section(mathomatic, equation, np, cp, allow_space);
	return cp;
}

/*
 * Parse variable name string pointed to by "cp".
 * The variable name is converted to Mathomatic format and stored in "*vp".
 *
 * If the variable is not special and never existed before, it is created.
 *
 * Return new string position if successful.
 * Display error message and return NULL on failure.
 */
char *
parse_var(MathoMatic* mathomatic, long *vp, char *cp)
{
	int	i, j;
	long	vtmp;
	char	buf[MAX_VAR_LEN+1];
	char	*cp1;
	int	len;
	int	level;		/* parentheses level */
	int	(*strcmpfunc)();

	if (mathomatic->case_sensitive_flag) {
		strcmpfunc = strcmp;
	} else {
		strcmpfunc = strcasecmp;
	}
	if (!isvarchar(mathomatic, *cp) || paren_increment(*cp) < 0) {
		error(mathomatic, _("Invalid variable."));
		return(NULL);	/* variable name must start with a valid variable character */
	}
	for (level = 0, cp1 = cp, i = 0; *cp1;) {
		if (level <= 0 && !isvarchar(mathomatic, *cp1)) {
			break;
		}
		j = paren_increment(*cp1);
		level += j;
		if (level < 0)
			break;
		if (i >= MAX_VAR_LEN) {
			error(mathomatic, _("Variable name too long."));
			return(NULL);
		}
		buf[i++] = *cp1++;
		if (j == -1 && level <= 0)
			break;
	}
	buf[i] = '\0';
	if (level > 0) {
		error(mathomatic, _("Unmatched parenthesis: missing )"));
		return(NULL);
	}

	if (strcasecmp(buf, NAN_NAME) == 0) {
		warning(mathomatic, _("Attempt to enter NaN (Not a Number); Converted to variable."));
	}
	if (strcasecmp(buf, "inf") == 0 || strcasecmp(buf, INFINITY_NAME) == 0) {
		error(mathomatic, _("Infinity cannot be used as a variable."));
		return(NULL);
	} else if ((*strcmpfunc)(buf, "sign") == 0) {
		vtmp = SIGN;
	} else {
		if (strncasecmp(cp, "i#", 2) == 0) {
			*vp = IMAGINARY;
			return(cp + 2);
		}
		if (strncasecmp(cp, "e#", 2) == 0) {
			*vp = V_E;
			return(cp + 2);
		}
		if (strncasecmp(cp, "pi#", 3) == 0) {
			*vp = V_PI;
			return(cp + 3);
		}
		for (level = 0, cp1 = cp, i = 0; *cp1;) {
			if (level <= 0 && !isvarchar(mathomatic, *cp1) && !isdigit(*cp1)) {
				break;
			}
			j = paren_increment(*cp1);
			level += j;
			if (level < 0)
				break;
			if (i >= MAX_VAR_LEN) {
				error(mathomatic, _("Variable name too long."));
				return(NULL);
			}
			buf[i++] = *cp1++;
			if (j == -1 && level <= 0)
				break;
		}
		if (i <= 0) {
			error(mathomatic, _("Empty variable name parsed!"));
			return(NULL);
		}
		buf[i] = '\0';
		if (level > 0) {
			error(mathomatic, _("Unmatched parenthesis: missing )"));
			return(NULL);
		}
		if ((*strcmpfunc)(buf, "i") == 0) {
			*vp = IMAGINARY;
			return(cp1);
		}
		if ((*strcmpfunc)(buf, "e") == 0) {
			*vp = V_E;
			return(cp1);
		}
		if ((*strcmpfunc)(buf, "pi") == 0) {
			*vp = V_PI;
			return(cp1);
		}
		if (is_all(buf)) {
			error(mathomatic, _("\"all\" is a reserved word and may not be used as a variable name."));
			return(NULL);
		}
		vtmp = 0;
		for (i = 0; mathomatic->var_names[i]; i++) {
			if ((*strcmpfunc)(buf, mathomatic->var_names[i]) == 0) {
				vtmp = i + VAR_OFFSET;
				break;
			}
		}
		if (vtmp == 0) {
			if (i >= (MAX_VAR_NAMES - 1)) {
				error(mathomatic, _("Maximum number of variable names reached."));
#if	!SILENT
				printf(_("Please restart or use \"clear all\".\n"));
#endif
				return(NULL);
			}
			len = strlen(buf) + 1;
			mathomatic->var_names[i] = (char *) malloc(len);
			if (mathomatic->var_names[i] == NULL) {
				error(mathomatic, _("Out of memory (can't malloc(3) variable name)."));
				return(NULL);
			}
			blt(mathomatic->var_names[i], buf, len);
			vtmp = i + VAR_OFFSET;
			mathomatic->var_names[i+1] = NULL;
		}
		*vp = vtmp;
		return cp1;
	}
/* for "sign" variables: */
	if (isdigit(*cp1)) {
		j = strtol(cp1, &cp1, 10);
		if (j < 0 || j > MAX_SUBSCRIPT) {
			error(mathomatic, _("Maximum subscript exceeded in special variable name."));
			return(NULL);
		}
		if (vtmp == SIGN) {
			mathomatic->sign_array[j+1] = true;
		}
		vtmp += ((long) (j + 1)) << VAR_SHIFT;
	} else if (vtmp == SIGN) {
		mathomatic->sign_array[0] = true;
	}
	*vp = vtmp;
	return cp1;
}

/*
 * Remove trailing spaces from a string.
 */
void
remove_trailing_spaces(char *cp)
{
	int	i;

	i = strlen(cp) - 1;
	while (i >= 0 && isspace(cp[i])) {
		cp[i] = '\0';
		i--;
	}
}

/*
 * This should be called for all line input.
 * Set point_flag to true if pointing to the input error will work for the passed string.
 * Truncate string to the actual content.
 */
void
set_error_level(MathoMatic* mathomatic, char *cp)
//char	*cp;	/* input string */
{
	char	*cp1;
	int	len;

	mathomatic->point_flag = true;
/* handle comments, line breaks, and the DOS EOF character (control-Z) by truncating the string where found */
	cp1 = cp;
	while ((cp1 = strpbrk(cp1, ";\n\r\032"))) {
		if (cp1 > cp && *(cp1 - 1) == '\\') {
			if (*cp1 == ';') {
/* skip backslash escaped semicolon */
				mathomatic->point_flag = false;
				len = strlen(cp1);
				blt(cp1 - 1, cp1, len + 1);
				continue;
			}
		}
		break;
	}
	if (cp1) {
		*cp1 = '\0';
	}
	remove_trailing_spaces(cp);
/* set point_flag to false if non-printable characters encountered */
	for (cp1 = cp; *cp1; cp1++) {
		if (!isprint(*cp1)) {
			mathomatic->point_flag = false;
		}
	}
}

/*
 * Return true if passed variable "v" is a constant.
 * Return value of constant in "*dp".
 */
int
var_is_const(v, dp)
long	v;
double	*dp;
{
	if (v == V_E) {
		if (dp)
			*dp = M_E;
		return true;
	}
	if (v == V_PI) {
		if (dp)
			*dp = M_PI;
		return true;
	}
	return false;
}

/*
 * Substitute E and PI variables with their respective constants.
 *
 * Return true if anything was substituted (therefore approximated).
 */
int
subst_constants(equation, np)
token_type	*equation;
int		*np;
{
	int	i;
	int	modified = false;
	double	d;

	for (i = 0; i < *np; i += 2) {
		if (equation[i].kind == VARIABLE) {
			if (var_is_const(equation[i].token.variable, &d)) {
				equation[i].kind = CONSTANT;
				equation[i].token.constant = d;
				modified = true;
			}
		}
	}
	return modified;
}

#ifndef	my_strlcpy
/*
 * A very efficient strlcpy(), which isn't available in some distributions, hence this code.
 *
 * Copy up to (n - 1) characters from string in src
 * to dest and null-terminate the result.
 *
 * Return length of src.
 */
int
my_strlcpy(dest, src, n)
char	*dest, *src;
int	n;
{
	int	len, len_src;

	len_src = strlen(src);
	if (n > 0) {
		len = min(n - 1, len_src);
		memmove(dest, src, len);
		dest[len] = '\0';
	}
	return len_src;
}
#endif
