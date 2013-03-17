/*
 * Standard routines for Mathomatic.
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
#if	UNIX || CYGWIN
#include <sys/ioctl.h>
#include <termios.h>
#endif

/*
 * Display the main Mathomatic startup message.
 * fp is the output file stream pointer it goes to, and should be stdout or gfp,
 * but really can go to any file you wish.
 */
void
display_startup_message(fp)
FILE	*fp;	/* output file pointer */
{
	long	es_size;

#if	SECURE
	fprintf(fp, _("Secure "));
#else
	if (security_level >= 2)
		fprintf(fp, _("Secure "));
	else if (security_level == -1)
		fprintf(fp, "m4 ");
#endif
	fprintf(fp, "Mathomatic version %s\n", VERSION);
	if (html_flag)
		fprintf(fp, "Copyright &copy; 1987-2012 George Gesslein II.\n");
	else
		fprintf(fp, "Copyright (C) 1987-2012 George Gesslein II.\n");
	es_size = (long) n_tokens * sizeof(token_type) * 2L / 1000L;
	if (es_size >= 1000) {
		fprintf(fp, _("%d equation spaces available in RAM; %ld megabytes per equation space.\n"),
		    N_EQUATIONS, (es_size + 500L) / 1000L);
	} else {
		fprintf(fp, _("%d equation spaces available in RAM; %ld kilobytes per equation space.\n"),
		    N_EQUATIONS, es_size);
	}
}

/*
 * Standard function to report an error to the user.
 */
void
error(str)
const char	*str;		/* constant string to display */
{
	error_str = str;	/* save reference to str, must be a constant string, temporary strings don't work */
#if	!SILENT && !LIBRARY
	set_color(2);		/* set color to red */
	printf("%s\n", str);
	default_color(false);	/* restore to default color */
#endif
}

/*
 * Reset last call to error(), as if it didn't happen.
 */
void
reset_error(void)
{
#if	!SILENT && !LIBRARY
	if (error_str)
		printf(_("Forgetting previous error.\n"));
#endif
	error_str = NULL;
}

/*
 * Standard function to report a warning only once to the user.
 * A warning is less serious than an error.
 */
void
warning(str)
const char	*str;		/* constant string to display */
{
	int	already_warned = false;

	if (warning_str) {
		if (strcmp(str, warning_str) == 0)
			already_warned = true;
	}
	warning_str = str;	/* save reference to str, must be a constant string, temporary strings don't work */
#if	!SILENT && !LIBRARY
	if (!already_warned && debug_level >= -1) {
		set_color(1);		/* set color to yellow */
		printf("Warning: %s\n", str);
		default_color(false);	/* restore to default color */
	}
#endif
}

/*
 * This is called when the maximum expression size has been exceeded.
 *
 * There is no return.
 */
void
error_huge(void)
{
	longjmp(jmp_save, 14);
}

/*
 * This is called when a bug test result is positive.
 *
 * There is no return.
 */
void
error_bug(str)
const char	*str;	/* constant string to display */
{
/* Return and display the passed error message in str. */
	error(str);	/* str must be a constant string, temporary strings don't work */
#if	SILENT || LIBRARY
	printf("%s\n", str);
#endif
	printf(_("Please report this bug to the maintainers,\n"));
	printf(_("along with the entry sequence that caused it.\n"));
#if	!LIBRARY
	printf(_("Type \"help bugs\" for info on how to report bugs found in this program.\n"));
#endif
	longjmp(jmp_save, 13);	/* Abort the current operation with the critical error number 13. */
}

/*
 * Check if a floating point math function flagged an error.
 *
 * There is no return if an error message is displayed.
 */
void
check_err(void)
{
	switch (errno) {
	case EDOM:
		errno = 0;
		if (domain_check) {
			domain_check = false;
		} else {
			error(_("Domain error in constant."));
			longjmp(jmp_save, 2);
		}
		break;
	case ERANGE:
		errno = 0;
		error(_("Floating point constant out of range."));
		longjmp(jmp_save, 2);
		break;
	}
}

/*
 * Get the current screen (window) width and height from the operating system.
 *
 * Return true if the global integers screen_columns and/or screen_rows were set.
 */
int
get_screen_size(void)
{
	int	rv = false;

#if	UNIX || CYGWIN
	struct winsize	ws;

	ws.ws_col = 0;
	ws.ws_row = 0;
	if (ioctl(1, TIOCGWINSZ, &ws) >= 0) {
		if (ws.ws_col > 0) {
			screen_columns = ws.ws_col;
			rv = true;
		}
		if (ws.ws_row > 0) {
			screen_rows = ws.ws_row;
			rv = true;
		}
	}
#else
	screen_columns = STANDARD_SCREEN_COLUMNS;
	screen_rows = STANDARD_SCREEN_ROWS;
	rv = true;
#endif
	return rv;
}

/*
 * Allocate the display lines in the vscreen[] array.
 * Call this before using vscreen[].
 *
 * Return true with vscreen[] allocated to TEXT_ROWS*current_columns characters if successful.
 */
int
malloc_vscreen(void)
{
	int	i;

	if (current_columns == 0 || ((screen_columns > 0) ? (current_columns != screen_columns) : (current_columns != TEXT_COLUMNS))) {
		if (screen_columns > 0) {
			current_columns = screen_columns;
		} else {
			current_columns = TEXT_COLUMNS;
		}
		for (i = 0; i < TEXT_ROWS; i++) {
			if (vscreen[i]) {
				free(vscreen[i]);
			}
			vscreen[i] = malloc(current_columns + 1);
			if (vscreen[i] == NULL) {
				error(_("Out of memory (can't malloc(3))."));
				current_columns = 0;
				return false;
			}
		}
	}
	return true;
}

/*
 * Allocate the needed global expression storage arrays.
 * Each is static and can hold n_tokens elements.
 * n_tokens must not change until Mathomatic terminates.
 *
 * init_mem() is called only once upon Mathomatic startup
 * before using the symbolic math engine,
 * and can be undone by calling free_mem() below.
 *
 * Returns true if successful, otherwise Mathomatic cannot be used.
 */
int
init_mem(void)
{
	if (n_tokens <= 0)
		return false;
	if ((scratch = (token_type *) malloc(((n_tokens * 3) / 2) * sizeof(token_type))) == NULL
	    || (tes = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL
	    || (tlhs = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL
	    || (trhs = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL) {
		return false;
	}
	if (alloc_next_espace() < 0) {	/* make sure there is at least 1 equation space */
		return false;
	}
	clear_all();
	return true;
}

#if	LIBRARY || VALGRIND
/*
 * Free the global expression storage arrays and other known memory buffers.
 * After calling this, memory usage is reset and Mathomatic becomes unusable,
 * so do not call unless finished with using the Mathomatic code.
 *
 * This routine is usually not needed, because when a program exits,
 * all the memory it allocated is released by the operating system.
 * Inclusion of this routine was requested by Tam Hanna for use with Symbian OS.
 */
void
free_mem(void)
{
	int	i;

	clear_all();

	free(scratch);
	free(tes);
	free(tlhs);
	free(trhs);

	for (i = 0; i < N_EQUATIONS; i++) {
		if (lhs[i]) {
			free(lhs[i]);
			lhs[i] = NULL;
		}
		if (rhs[i]) {
			free(rhs[i]);
			rhs[i] = NULL;
		}
	}
	n_equations = 0;

	for (i = 0; i < TEXT_ROWS; i++) {
		if (vscreen[i]) {
			free(vscreen[i]);
			vscreen[i] = NULL;
		}
	}
	current_columns = 0;
}
#endif

#if	DEBUG
/*
 * Use this function to check for any erroneous global conditions.
 * Use of this function is rather paranoid, but helpful.
 *
 * Always returns true, or doesn't return on error.
 */
int
check_gvars(void)
{
	if (!(domain_check == false &&
	high_prec == false &&
	partial_flag == true &&
	symb_flag == false &&
	sign_cmp_flag == false &&
	approximate_roots == false))
		error_bug("Global vars got changed!");

	if (!(zero_token.level == 1 &&
	zero_token.kind == CONSTANT &&
	zero_token.token.constant == 0.0 &&
	one_token.level == 1 &&
	one_token.kind == CONSTANT &&
	one_token.token.constant == 1.0))
		error_bug("Global constants got changed!");

	return true;
}
#endif

/*
 * Initialize some important global variables to their defaults.
 * This is called on startup and by process() to reset the global flags to the default state.
 * This is also called when processing is aborted with a longjmp(3).
 */
void
init_gvars(void)
{
	domain_check = false;
	high_prec = false;
	partial_flag = true;
	symb_flag = false;
	sign_cmp_flag = false;
	approximate_roots = false;
	repeat_flag = false;

	/* initialize the universal and often used constant "0" expression */
	zero_token.level = 1;
	zero_token.kind = CONSTANT;
	zero_token.token.constant = 0.0;

	/* initialize the universal and often used constant "1" expression */
	one_token.level = 1;
	one_token.kind = CONSTANT;
	one_token.token.constant = 1.0;
}

/*
 * Clean up when processing is unexpectedly interrupted or terminated.
 */
void
clean_up(void)
{
	int	i;

	init_gvars();		/* reset the global variables to the default */
	if (gfp != default_out) {	/* reset the output file to default */
#if	!SECURE
		if (gfp != stdout && gfp != stderr)
			fclose(gfp);
#endif
		gfp = default_out;
	}
	gfp_filename = NULL;
	for (i = 0; i < n_equations; i++) {
		if (n_lhs[i] <= 0) {
			n_lhs[i] = 0;
			n_rhs[i] = 0;
		}
	}
}

/*
 * Register all sign variables in all equation spaces
 * so that the next sign variables returned by next_sign() will be unique.
 */
void
set_sign_array(void)
{
	int	i, j;

	CLEAR_ARRAY(sign_array);
	for (i = 0; i < n_equations; i++) {
		if (n_lhs[i] > 0) {
			for (j = 0; j < n_lhs[i]; j += 2) {
				if (lhs[i][j].kind == VARIABLE && (lhs[i][j].token.variable & VAR_MASK) == SIGN) {
					sign_array[(lhs[i][j].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
			for (j = 0; j < n_rhs[i]; j += 2) {
				if (rhs[i][j].kind == VARIABLE && (rhs[i][j].token.variable & VAR_MASK) == SIGN) {
					sign_array[(rhs[i][j].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
		}
	}
}

/*
 * Return next unused sign variable in "*vp".
 * Mark it used.
 */
int
next_sign(vp)
long	*vp;
{
	int	i;

	for (i = 0;; i++) {
		if (i >= ARR_CNT(sign_array)) {
			/* out of unique sign variables */
			*vp = SIGN;
			return false;
		}
		if (!sign_array[i]) {
			*vp = SIGN + (((long) i) << VAR_SHIFT);
			sign_array[i] = true;
			break;
		}
	}
	return true;
}

/*
 * Erase all equation spaces and initialize the global variables.
 * Similar to a restart.
 */
void
clear_all(void)
{
	int	i;

/* select first equation space */
	cur_equation = 0;
/* erase all equation spaces by setting their length to zero */
	CLEAR_ARRAY(n_lhs);
	CLEAR_ARRAY(n_rhs);
/* forget all variables names */
	for (i = 0; var_names[i]; i++) {
		free(var_names[i]);
		var_names[i] = NULL;
	}
/* reset everything to a known state */
	CLEAR_ARRAY(sign_array);
	init_gvars();
}

/*
 * Return true if the specified equation space is available,
 * zeroing and allocating if necessary.
 */
int
alloc_espace(i)
int	i;	/* equation space number */
{
	if (i < 0 || i >= N_EQUATIONS)
		return false;
	n_lhs[i] = 0;
	n_rhs[i] = 0;
	if (lhs[i] && rhs[i])
		return true;	/* already allocated */
	if (lhs[i] || rhs[i])
		return false;	/* something is wrong */
	lhs[i] = (token_type *) malloc(n_tokens * sizeof(token_type));
	if (lhs[i] == NULL)
		return false;
	rhs[i] = (token_type *) malloc(n_tokens * sizeof(token_type));
	if (rhs[i] == NULL) {
		free(lhs[i]);
		lhs[i] = NULL;
		return false;
	}
	return true;
}

/*
 * Allocate all equation spaces up to and including an equation space number,
 * making sure the specified equation number is valid and usable.
 *
 * Returns true if successful.
 */
int
alloc_to_espace(en)
int	en;	/* equation space number */
{
	if (en < 0 || en >= N_EQUATIONS)
		return false;
	for (;;) {
		if (en < n_equations)
			return true;
		if (n_equations >= N_EQUATIONS)
			return false;
		if (!alloc_espace(n_equations)) {
			warning(_("Memory is exhausted."));
			return false;
		}
		n_equations++;
	}
}

/*
 * Allocate or reuse an empty equation space.
 *
 * Returns empty equation space number ready for use or -1 on error.
 */
int
alloc_next_espace(void)
{
	int	i, n;

	for (n = cur_equation, i = 0;; n = (n + 1) % N_EQUATIONS, i++) {
		if (i >= N_EQUATIONS)
			return -1;
		if (n >= n_equations) {
			n = n_equations;
			if (!alloc_espace(n)) {
				warning(_("Memory is exhausted."));
				for (n = 0; n < n_equations; n++) {
					if (n_lhs[n] == 0) {
						n_rhs[n] = 0;
						return n;
					}
				}
				return -1;
			}
			n_equations++;
			return n;
		}
		if (n_lhs[n] == 0)
			break;
	}
	n_rhs[n] = 0;
	return n;
}

/*
 * Return the number of the next empty equation space, otherwise don't return.
 */
int
next_espace(void)
{
	int		i, j;
	long		answer_v = 0;		/* Mathomatic answer variable */

	i = alloc_next_espace();
	if (i < 0) {
#if	!SILENT
		printf(_("Deleting old numeric calculations to free up equation spaces.\n"));
#endif
		parse_var(&answer_v, "answer");	/* convert to a Mathomatic variable */
		for (j = 0; j < n_equations; j++) {
			if (n_lhs[j] == 1 && lhs[j][0].kind == VARIABLE
			    && lhs[j][0].token.variable == answer_v) {
				/* delete calculation from memory */
				n_lhs[j] = 0;
				n_rhs[j] = 0;
			}
		}
		i = alloc_next_espace();
		if (i < 0) {
			error(_("Out of free equation spaces."));
#if	!SILENT
			printf(_("Use the clear command on unnecessary equations and try again.\n"));
#endif
			longjmp(jmp_save, 3);	/* do not return */
		}
	}
	return i;
}

/*
 * Copy equation space "src" to equation space "dest".
 * "dest" is overwritten.
 */
void
copy_espace(src, dest)
int	src, dest;	/* equation space numbers */
{
	if (src == dest) {
#if	DEBUG
		error_bug("Internal error: copy_espace() source and destination the same.");
#endif
		return;
	}
	blt(lhs[dest], lhs[src], n_lhs[src] * sizeof(token_type));
	n_lhs[dest] = n_lhs[src];
	blt(rhs[dest], rhs[src], n_rhs[src] * sizeof(token_type));
	n_rhs[dest] = n_rhs[src];
}

/*
 * Return true if equation space "i" is a valid equation solved for a normal variable.
 */
int
solved_equation(i)
int	i;
{
	if (empty_equation_space(i))
		return false;
	if (n_rhs[i] <= 0)
		return false;
	if (n_lhs[i] != 1 || lhs[i][0].kind != VARIABLE || (lhs[i][0].token.variable & VAR_MASK) <= SIGN)
		return false;
	if (found_var(rhs[i], n_rhs[i], lhs[i][0].token.variable))
		return false;
	return true;
}

/*
 * Return the number of times variable "v" is found in an expression.
 */
int
found_var(p1, n, v)
token_type	*p1;	/* expression pointer */
int		n;	/* expression length */
long		v;	/* standard Mathomatic variable */
{
	int	j;
	int	count = 0;

	if (v) {
		for (j = 0; j < n; j++) {
			if (p1[j].kind == VARIABLE && p1[j].token.variable == v) {
				count++;
			}
		}
	}
	return count;
}

/*
 * Return true if variable "v" exists in equation space "i".
 */
int
var_in_equation(i, v)
int	i;	/* equation space number */
long	v;	/* standard Mathomatic variable */
{
	if (empty_equation_space(i))
		return false;
	if (found_var(lhs[i], n_lhs[i], v))
		return true;
	if (n_rhs[i] <= 0)
		return false;
	if (found_var(rhs[i], n_rhs[i], v))
		return true;
	return false;
}

/*
 * Return true if variable "v" exists in any equation space.
 *
 * Search forward starting at the next equation space if forward_direction is true,
 * otherwise search backwards starting at the previous equation space.
 * If found, return true with cur_equation set to the equation space the variable is found in.
 */
int
search_all_for_var(v, forward_direction)
long	v;
int	forward_direction;
{
	int	i, n;

	i = cur_equation;
	for (n = 0; n < n_equations; n++) {
		if (forward_direction) {
			if (i >= (n_equations - 1))
				i = 0;
			else
				i++;
		} else {
			if (i <= 0)
				i = n_equations - 1;
			else
				i--;
		}
		if (var_in_equation(i, v)) {
			cur_equation = i;
			return true;
		}
	}
	return false;
}

/*
 * Replace all occurrences of variable from_v with to_v in an equation space.
 */
void
rename_var_in_es(en, from_v, to_v)
int	en;	 	/* equation space number */
long	from_v, to_v;	/* Mathomatic variables */
{
	int	i;

	if (empty_equation_space(en)) {
#if	DEBUG
		error_bug("Invalid or empty equation number given to rename_var_in_es().");
#else
		return;
#endif
	}
	for (i = 0; i < n_lhs[en]; i += 2)
		if (lhs[en][i].kind == VARIABLE
		    && lhs[en][i].token.variable == from_v)
			lhs[en][i].token.variable = to_v;
	for (i = 0; i < n_rhs[en]; i += 2)
		if (rhs[en][i].kind == VARIABLE
		    && rhs[en][i].token.variable == from_v)
			rhs[en][i].token.variable = to_v;
}

/*
 * Substitute every instance of "v" in "equation" with "expression".
 *
 * Return true if something was substituted.
 */
int
subst_var_with_exp(equation, np, expression, len, v)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to equation side length */
token_type	*expression;	/* expression pointer */
int		len;		/* expression length */
long		v;		/* variable to substitute with expression */
{
	int	j, k;
	int	level;
	int	substituted = false;

	if (v == 0 || len <= 0)
		return false;
	for (j = *np - 1; j >= 0; j--) {
		if (equation[j].kind == VARIABLE && equation[j].token.variable == v) {
			level = equation[j].level;
			if (*np + len - 1 > n_tokens) {
				error_huge();
			}
			if (len > 1) {
				blt(&equation[j+len], &equation[j+1], (*np - (j + 1)) * sizeof(token_type));
				*np += len - 1;
			}
			blt(&equation[j], expression, len * sizeof(token_type));
			for (k = j; k < j + len; k++)
				equation[k].level += level;
			substituted = true;
		}
	}
	if (substituted) {
		if (is_integer_var(v) && !is_integer_expr(expression, len)) {
			warning(_("Substituting integer variable with non-integer expression."));
		}
	}
	return substituted;
}

/*
 * Return the base (minimum) parentheses level encountered in a Mathomatic "expression".
 */
int
min_level(expression, n)
token_type	*expression;	/* expression pointer */
int		n;		/* expression length */
{
	int		min1;
	token_type	*p1, *ep;

#if	DEBUG
	if (expression == NULL)
		error_bug("NULL pointer passed to min_level().");
#endif
	switch (n) {
	case 1:
		return expression[0].level;
	case 3:
		return expression[1].level;
	default:
		if (n <= 0 || (n & 1) != 1)
			error_bug("Invalid expression length in call to min_level().");
		break;
	}
	min1 = expression[1].level;
	ep = &expression[n];
	for (p1 = &expression[3]; p1 < ep; p1 += 2) {
		if (p1->level < min1)
			min1 = p1->level;
	}
	return min1;
}

/*
 * Get default equation number from a command parameter string.
 * The equation number must be the only parameter.
 * If no equation number is specified, default to the current equation.
 *
 * Return -1 on error.
 */
int
get_default_en(cp)
char	*cp;
{
	int	i;

	if (*cp == '\0') {
		i = cur_equation;
	} else {
		i = decstrtol(cp, &cp) - 1;
		if (extra_characters(cp))
			return -1;
	}
	if (not_defined(i)) {
		return -1;
	}
	return i;
}

/*
 * Get an expression from the user.
 * The prompt must be previously copied into the global prompt_str[].
 *
 * Return true if successful.
 */
int
get_expr(equation, np)
token_type	*equation;	/* where the parsed expression is stored (equation side) */
int		*np;		/* pointer to the returned parsed expression length */
{
	char	buf[DEFAULT_N_TOKENS];
	char	*cp;

#if	LIBRARY
	snprintf(buf, sizeof(buf), "#%+d", pull_number);
	pull_number++;
	cp = parse_expr(equation, np, buf, true);
	if (extra_characters(cp))
		return false;
	return(cp && *np > 0);
#else
	for (;;) {
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		cp = parse_expr(equation, np, cp, true);
		if (cp && !extra_characters(cp)) {
			break;
		}
	}
	return(*np > 0);
#endif
}

/*
 * Prompt for a variable name from the user.
 *
 * Return true if successful.
 */
int
prompt_var(vp)
long	*vp;	/* pointer to the returned variable */
{
	char	buf[MAX_CMD_LEN];
	char	*cp;

	for (;;) {
		my_strlcpy(prompt_str, _("Enter variable: "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		if (*cp == '\0') {
			return false;
		}
		cp = parse_var2(vp, cp);
		if (cp == NULL || extra_characters(cp)) {
			continue;
		}
		return true;
	}
}

/*
 * Return true and display a message if equation "i" is undefined.
 */
int
not_defined(i)
int	i;	/* equation space number */
{
	if (i < 0 || i >= n_equations) {
		error(_("Invalid equation number."));
		return true;
	}
	if (n_lhs[i] <= 0) {
		if (i == cur_equation) {
			error(_("Current equation space is empty."));
		} else {
			error(_("Equation space is empty."));
		}
		return true;
	}
	return false;
}

/*
 * Verify that a current equation or expression is loaded.
 *
 * Return true and display a message if the current equation space is empty or not defined.
 */
int
current_not_defined(void)
{
	int	i;

	i = cur_equation;
	if (i < 0 || i >= n_equations) {
		error(_("Current equation number out of range; reset to 1."));
		i = cur_equation = 0;
	}
	if (n_lhs[i] <= 0) {
		error(_("No current equation or expression."));
		return true;
	}
	return false;
}

/*
 * Common routine to output the prompt in prompt_str[] and get a line of input from stdin.
 * All Mathomatic input comes from this routine, except for file reading.
 * If there is no more input (EOF), exit this program with no error.
 *
 * Returns "string" if successful or NULL on error.
 */
char *
get_string(string, n)
char	*string;	/* storage for input string */
int	n;		/* maximum size of "string" in bytes */
{
#if	LIBRARY
	error(_("Library usage error. Input requested, possibly due to missing command-line argument."));
	return NULL;
#else
	int	i;
#if	READLINE || EDITLINE
	char	*cp;
#endif

#if	DEBUG
	if (string == NULL)
		error_bug("NULL pointer passed to get_string().");
#endif
	if (quiet_mode) {
		prompt_str[0] = '\0';	/* don't display a prompt */
	}
	input_column = strlen(prompt_str);
	fflush(NULL);	/* flush everything before gathering input */
#if	READLINE || EDITLINE
	if (readline_enabled) {
		cp = readline(prompt_str);
		if (cp == NULL) {
			if (!quiet_mode)
				printf(_("\nEnd of input.\n"));
			exit_program(0);
		}
		my_strlcpy(string, cp, n);
		if (skip_space(cp)[0] && (last_history_string == NULL || strcmp(last_history_string, cp))) {
			add_history(cp);
			last_history_string = cp;
		} else {
			free(cp);
		}
	} else {
		printf("%s", prompt_str);
		fflush(stdout);
		if (fgets(string, n, stdin) == NULL) {
			if (!quiet_mode)
				printf(_("\nEnd of input.\n"));
			exit_program(0);
		}
	}
#else
	printf("%s", prompt_str);
	fflush(stdout);
	if (fgets(string, n, stdin) == NULL) {
		if (!quiet_mode)
			printf(_("\nEnd of input.\n"));
		exit_program(0);
	}
#endif
	if (abort_flag) {
		abort_flag = false;
		longjmp(jmp_save, 13);
	}
	/* Fix an fgets() peculiarity: */
	i = strlen(string) - 1;
	if (i >= 0 && string[i] == '\n') {
		string[i] = '\0';
	}
	if ((gfp != stdout && gfp != stderr) || (echo_input && !quiet_mode)) {
		/* Input that is prompted for is now included in the redirected output
		   of a command to a file, making redirection like logging. */
		fprintf(gfp, "%s%s\n", prompt_str, string);
	}
	set_error_level(string);
	abort_flag = false;
	return string;
#endif
}

/*
 * Display the prompt in prompt_str[] and wait for "y" or "n" followed by Enter.
 *
 * Return true if "y".
 * Return false if "n" or not interactive.
 */
int
get_yes_no(void)
{
	char	*cp;
	char	buf[MAX_CMD_LEN];

#if	0
	if (!isatty(0)) {
		return false;
	}
#endif
	for (;;) {
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		str_tolower(cp);
		switch (*cp) {
		case 'n':
			return false;
		case 'y':
			return true;
		}
	}
}

/*
 * Display the result of a command,
 * or store the pointer to the text of the listed expression
 * into result_str if compiled for the library.
 *
 * Return true if successful.
 */
int
return_result(en)
int	en;	/* equation space number the result is in */
{
	if (empty_equation_space(en)) {
		return false;
	}
#if	LIBRARY
	make_fractions_and_group(en);
	if (factor_int_flag) {
		factor_int_equation(en);
	}
	free_result_str();
#if	1	/* Set this to 1 to allow display2d to decide library output mode. */
	if (display2d) {
		result_str = flist_equation_string(en);
		if (result_str == NULL)
			result_str = list_equation(en, false);
	} else {
		result_str = list_equation(en, false);
	}
#else	/* For feeding command output to the next command's input only. */
	result_str = list_equation(en, false);
#endif
	result_en = en;
	if (gfp == stdout) {
		return(result_str != NULL);
	}
#endif
	return(list_sub(en) != 0);
}

/*
 * Free any malloc()ed result_str, so there won't be a memory leak
 * in the symbolic math library.
 */
void
free_result_str(void)
{
	if (result_str) {
		free(result_str);
		result_str = NULL;
	}
	result_en = -1;
}

/*
 * Return true if the first word in the passed string is "all".
 */
int
is_all(cp)
char	*cp;
{
	return(strcmp_tospace(cp, "all") == 0);
}

/*
 * Process an equation number range given in text string "*cpp".
 * Skip past all spaces and update "*cpp" to point to the next argument if successful.
 * If no equation number or range is given, or it is invalid, assume the current equation is wanted and
 * don't skip anything.
 *
 * Return true if successful,
 * with the starting equation number in "*ip"
 * and ending equation number in "*jp".
 */
int
get_range(cpp, ip, jp)
char	**cpp;
int	*ip, *jp;
{
	int	i;
	char	*cp;
	int	rv;

	cp = skip_comma_space(*cpp);
	if (is_all(cp)) {
		cp = skip_param(cp);
		*ip = 0;
		*jp = n_equations - 1;
		while (*jp > 0 && n_lhs[*jp] == 0)
			(*jp)--;
	} else {
		if (*cp == '0') {
			goto use_current;
		}
		if (isdigit(*cp)) {
			*ip = strtol(cp, &cp, 10) - 1;
		} else {
			*ip = cur_equation;
		}
		if (*cp != '-') {
			if (*cp == '\0' || *cp == ',' || isspace(*cp)) {
				if (not_defined(*ip)) {
					return false;
				}
				*jp = *ip;
				*cpp = skip_comma_space(cp);
				return true;
			} else {
use_current:
				*jp = *ip = cur_equation;
#if	1
				rv = !empty_equation_space(cur_equation);	/* don't display error message */
				if (rv) {
					debug_string(1, _("Defaulting to the current equation space."));
				} else {
					debug_string(1, _("Defaulting to current empty equation space."));
				}
#else
				rv = !current_not_defined();		/* display an error message if error */
				if (rv) {
					debug_string(1, _("Defaulting to the current equation space."));
				}
#endif
				return rv;
			}
		}
		(cp)++;
		if (*cp == '0') {
			goto use_current;
		}
		if (isdigit(*cp)) {
			*jp = strtol(cp, &cp, 10) - 1;
		} else {
			*jp = cur_equation;
		}
		if (*cp && !isspace(*cp)) {
			goto use_current;
		}
		if (*ip < 0 || *ip >= N_EQUATIONS || *jp < 0 || *jp >= N_EQUATIONS) {
			error(_("Invalid equation number (out of range)."));
			return false;
		}
		if (*jp < *ip) {
			i = *ip;
			*ip = *jp;
			*jp = i;
		}
	}
	cp = skip_comma_space(cp);
	for (i = *ip; i <= *jp; i++) {
		if (n_lhs[i] > 0) {
			*cpp = cp;
			return true;
		}
	}
	error(_("No expressions defined in specified range."));
	return false;
}

/*
 * This function is provided to make sure there is nothing else on a command line.
 *
 * Returns true if any non-space characters are encountered before the end of the string
 * and an error message is printed.
 * Otherwise just returns false indicating everything is OK.
 */
int
extra_characters(cp)
char	*cp;	/* command line string */
{
	if (cp) {
		cp = skip_comma_space(cp);
		if (*cp) {
			printf(_("\nError: \"%s\" not required on input line.\n"), cp);
			error(_("Extra characters or unrecognized argument."));
			return true;
		}
	}
	return false;
}

/*
 * get_range() if it is the last possible option on the command line,
 * otherwise display an error message and return false.
 */
int
get_range_eol(cpp, ip, jp)
char	**cpp;
int	*ip, *jp;
{
	if (!get_range(cpp, ip, jp)) {
		return false;
	}
	if (extra_characters(*cpp)) {
		return false;
	}
	return true;
}

/*
 * Skip over space characters.
 */
char *
skip_space(cp)
char	*cp;	/* character pointer */
{
	if (cp) {
		while (*cp && isspace(*cp))
			cp++;
	}
	return cp;
}

/*
 * Skip over a possible comma and space characters.
 */
char *
skip_comma_space(cp)
char	*cp;	/* character pointer */
{
	if (cp) {
		cp = skip_space(cp);
		if (*cp == ',')
			cp = skip_space(cp + 1);
	}
	return cp;
}

/*
 * Enhanced decimal strtol().
 * Skips trailing spaces or commas.
 */
long
decstrtol(cp, cpp)
char	*cp, **cpp;
{
	long	l;

	l = strtol(cp, cpp, 10);
	if (cpp && *cpp && cp != *cpp) {
		*cpp = skip_comma_space(*cpp);
	}
	return l;
}

/*
 * Return true if passed character is a Mathomatic command parameter delimiter.
 */
int
isdelimiter(ch)
int	ch;
{
	return(isspace(ch) || ch == ',' || ch == '=');
}

/*
 * Skip over the current parameter in a Mathomatic command line string.
 * Parameters are usually separated with spaces or a comma or equals sign.
 *
 * Returns a string (character pointer) to the next parameter.
 */
char *
skip_param(cp)
char	*cp;
{
	if (cp) {
		while (*cp && (!isascii(*cp) || !isdelimiter(*cp))) {
			cp++;
		}
		cp = skip_space(cp);
		if (*cp && isdelimiter(*cp)) {
			cp = skip_space(cp + 1);
		}
	}
	return(cp);
}

/*
 * Compare strings up to the end or the first space or parameter delimiter,
 * ignoring alphabetic case.
 *
 * Returns zero on exact match, otherwise non-zero if strings are different.
 */
int
strcmp_tospace(cp1, cp2)
char	*cp1, *cp2;
{
	char	*cp1a, *cp2a;

#if	DEBUG
	if (cp1 == NULL || cp2 == NULL)
		error_bug("NULL pointer passed to strcmp_tospace().");
#endif
	for (cp1a = cp1; *cp1a && !isdelimiter(*cp1a); cp1a++)
		;
	for (cp2a = cp2; *cp2a && !isdelimiter(*cp2a); cp2a++)
		;
	return strncasecmp(cp1, cp2, max(cp1a - cp1, cp2a - cp2));
}

/*
 * Return the number of "level" additive type operators.
 */
int
level_plus_count(p1, n1, level)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
int		level;	/* parentheses level number to check */
{
	int	i;
	int	count = 0;

	for (i = 1; i < n1; i += 2) {
		if (p1[i].level == level) {
			switch (p1[i].token.operatr) {
			case PLUS:
			case MINUS:
				count++;
			}
		}
	}
	return count;
}

/*
 * Return the number of level 1 additive type operators.
 */
int
level1_plus_count(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	return level_plus_count(p1, n1, min_level(p1, n1));
}

/*
 * Return the count of variables in an expression.
 */
int
var_count(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;
	int	count = 0;

	for (i = 0; i < n1; i += 2) {
		if (p1[i].kind == VARIABLE) {
			count++;
		}
	}
	return count;
}

/*
 * Set "*vp" if single variable expression.
 *
 * Return true if expression contains no variables.
 */
int
no_vars(source, n, vp)
token_type	*source;	/* expression pointer */
int		n;		/* expression length */
long		*vp;		/* variable pointer */
{
	int	j;
	int	found = false;

	if (*vp) {
		return(var_count(source, n) == 0);
	}
	for (j = 0; j < n; j += 2) {
		if (source[j].kind == VARIABLE) {
			if ((source[j].token.variable & VAR_MASK) <= SIGN)
				continue;
			if (*vp) {
				if (*vp != source[j].token.variable) {
					*vp = 0;
					break;
				}
			} else {
				found = true;
				*vp = source[j].token.variable;
			}
		}
	}
	return(!found);
}

/*
 * Return true if expression contains infinity or NaN (Not a Number).
 */
int
exp_contains_infinity(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == CONSTANT && !isfinite(p1[i].token.constant)) {
			return true;
		}
	}
	return false;
}

/*
 * Return true if expression contains NaN (Not a Number).
 */
int
exp_contains_nan(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == CONSTANT && isnan(p1[i].token.constant)) {
			return true;
		}
	}
	return false;
}

/*
 * Return true if expression is numeric (not symbolic).
 * Pseudo-variables e, pi, i, and sign are considered numeric.
 */
int
exp_is_numeric(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == VARIABLE && (p1[i].token.variable & VAR_MASK) > SIGN) {
			return false;		/* not numerical (contains a variable) */
		}
	}
	return true;
}

/*
 * Test if expression contains an absolute value.
 * Return true if it does.
 */
int
exp_is_absolute(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;
	int	level;

	for (i = n1 - 2; i > 2; i -= 2) {
		if (p1[i].token.operatr != POWER)
			continue;
		level = p1[i].level;
		if (p1[i+1].level == level && p1[i+1].kind == CONSTANT && fmod(p1[i+1].token.constant, 1.0) != 0.0) {
			level++;
			if (p1[i-2].token.operatr == POWER && p1[i-2].level == level && p1[i-1].level == level && p1[i-1].kind == CONSTANT) {
				return true;
			}
		}
	}
	return false;
}

/*
 * Check for division by zero.
 *
 * Display a warning and return true if passed double is 0.
 */
int
check_divide_by_zero(denominator)
double	denominator;
{
	if (denominator == 0) {
		warning(_("Division by zero."));
		return true;
	}
	return false;
}

#if	CYGWIN || MINGW
/*
 * dirname(3) function for Microsoft Windows.
 * dirname(3) strips the non-directory suffix from a filename.
 */
char *
dirname_win(cp)
char	*cp;	/* string containing filename to modify */
{
	int	i;

	if (cp == NULL)
		return(".");
	i = strlen(cp);
	while (i >= 0 && cp[i] != '\\' && cp[i] != '/')
		i--;
	if (i < 0)
		return(".");
	cp[i] = '\0';
	return(cp);
}
#endif

#if	!SECURE
/*
 * Load set options from startup file "~/.mathomaticrc".
 *
 * Return false if there was an error reading the startup file,
 * otherwise return true.
 */
int
load_rc(return_true_if_no_file, ofp)
int	return_true_if_no_file;
FILE	*ofp;	/* if non-NULL, display each line as read in to this file */
{
	FILE	*fp = NULL;
	char	buf[MAX_CMD_LEN];
	char	*cp;
	int	rv = true;

	cp = getenv("HOME");
	if (cp) {
		snprintf(rc_file, sizeof(rc_file), "%s/%s", cp, ".mathomaticrc");
		fp = fopen(rc_file, "r");
	}
#if	CYGWIN || MINGW
	if (fp == NULL && cp) {
		snprintf(rc_file, sizeof(rc_file), "%s/%s", cp, "mathomatic.rc");
		fp = fopen(rc_file, "r");
	}
	if (fp == NULL && dir_path) {
		snprintf(rc_file, sizeof(rc_file), "%s/%s", dir_path, "mathomatic.rc");
		fp = fopen(rc_file, "r");
	}
#endif
	if (fp == NULL) {
		if (return_true_if_no_file) {
			return true;
		} else {
			perror(rc_file);
			return false;
		}
	}
	if (!quiet_mode && !eoption) {
		printf(_("Loading startup set options from \"%s\".\n"), rc_file);
	}
	while ((cp = fgets(buf, sizeof(buf), fp)) != NULL) {
		if (ofp)
			fprintf(ofp, "%s", cp);
		set_error_level(cp);
		if (!set_options(cp, true))
			rv = false;
	}
	if (fclose(fp)) {
		rv = false;
		perror(rc_file);
	}
	return rv;
}

#if	0	/* not currently used */
/*
 * Display set options from startup file "~/.mathomaticrc".
 *
 * Return false if there was an error reading the startup file,
 * otherwise return true.
 */
int
display_rc(ofp)
FILE	*ofp;
{
	FILE	*fp = NULL;
	char	buf[MAX_CMD_LEN];
	char	*cp;
	int	rv = true;

	printf(_("Displaying startup set options from \"%s\":\n\n"), rc_file);
	fp = fopen(rc_file, "r");
	if (fp == NULL) {
		perror(rc_file);
		return false;
	}
	while ((cp = fgets(buf, sizeof(buf), fp)) != NULL) {
		fprintf(ofp, "%s", cp);
	}
	if (fclose(fp)) {
		rv = false;
		perror(rc_file);
	}
	return rv;
}
#endif
#endif
