/*
 * Mathomatic commands that don't belong anywhere else.
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

#define	OPT_MIN_SIZE	7	/* Minimum size (in tokens) of repeated expressions to find in optimize command. */

enum spf_function {
	SUM_COMMAND,
	PRODUCT_COMMAND,
	FOR_COMMAND
};

static int sum_product(MathoMatic* mathomatic, char *cp, enum spf_function current_function);
static int complex_func(MathoMatic* mathomatic, char *cp, int imag_flag);
static int elim_sub(MathoMatic* mathomatic, int i, long v);


#if	SHELL_OUT
/*
 * The plot command.
 *
 * All command functions like this return true if successful, or false for failure.
 */
int
plot_cmd(MathoMatic* mathomatic, char *cp)
//char	*cp;	/* the command-line argument */
{
#define	APPEND(str)	{ if (strlen(str) + cl1_len < sizeof(cl1)) { strcpy(&cl1[cl1_len], str); cl1_len += strlen(str); } else warning(mathomatic, _("Expression too large to plot; omitted.")); }

	int		i1, i2;
	int		start, stop;
	int		first_time = true;
	int		cl1_len = 0, cl2_len = 0, len;
	char		*cp1, cl[16384+MAX_CMD_LEN], cl1[16384], cl2[MAX_CMD_LEN], *exp_str;
	long		v, vx;		/* Mathomatic variables */
	token_type	*equation;
	int		*np;
	int		ev;		/* exit value */
	int		cur_equation_flag = false;

	if (mathomatic->security_level > 0) {
		mathomatic->show_usage = false;
		error(mathomatic, _("Command disabled by security level."));
		return false;
	}
	if (!parse_var(mathomatic, &vx, "x")) {
		return false;
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &start, &stop)) {
			reset_error(mathomatic);
			break;
		}
		if (cp != cp1 || first_time) {
			if (cp == cp1)
				cur_equation_flag = (!empty_equation_space(mathomatic, mathomatic->cur_equation));
			for (i1 = start; i1 <= stop; i1++) {
				if (i1 != mathomatic->cur_equation) {
					cur_equation_flag = false;
				}
				if (mathomatic->n_lhs[i1] > 0) {
					v = 0;
					if (mathomatic->n_rhs[i1]) {
						equation = mathomatic->rhs[i1];
						np = &mathomatic->n_rhs[i1];
					} else {
						equation = mathomatic->lhs[i1];
						np = &mathomatic->n_lhs[i1];
					}
					if (!no_vars(equation, *np, &v) && v) {
						if (strcmp(var_name(mathomatic, v), "x") && strcmp(var_name(mathomatic, v), "t")) {
							list_var(mathomatic, v, 0);
							fprintf(mathomatic->gfp, _("#%d: Renaming variable %s to x for gnuplot.\n"), i1 + 1, mathomatic->var_str);
							rename_var_in_es(mathomatic, i1, v, vx);
						}
					}
					if (mathomatic->n_rhs[i1] && !solved_equation(mathomatic, i1)) {
						warning(mathomatic, _("Not a normally solved equation, plotting the RHS only."));
					}
					for (i2 = 0; i2 < *np; i2 += 2) {
						if (equation[i2].kind == VARIABLE && (equation[i2].token.variable & VAR_MASK) == SIGN) {
							warning(mathomatic, _("Plot expression contains sign variables; try \"simplify sign\" before plotting."));
							break;
						}
					}
					exp_str = list_expression(mathomatic, equation, *np, 3);
					if (exp_str == NULL)
						return false;
					if (cl1_len) {
						APPEND(", ");
					}
					APPEND(exp_str);
					free(exp_str);
				}
			}
		}
		first_time = false;
	} while (*cp && cp != cp1);
	cl1[cl1_len] = '\0';
	if (cl1_len == 0 && *cp == '\0') {
		error(mathomatic, _("No plot expression specified."));
		return false;
	}
	for (cl2_len = 0, i2 = 0; cp[i2]; i2++) {
		if ((cl2_len + 2) >= sizeof(cl2)) {
			error(mathomatic, _("Command-line too long."));
			return false;
		}
		switch (cp[i2]) {
		case '^':
			cl2[cl2_len] = '*';
			cl2_len++;
			cl2[cl2_len] = '*';
			cl2_len++;
			break;
		default:
			cl2[cl2_len] = cp[i2];
			cl2_len++;
			break;
		}
	}
	cl2[cl2_len] = '\0';
	if (cl1_len && cl2_len && cl2[cl2_len-1] != ',') {
		if (cur_equation_flag) {
			snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("Do you want to plot the current equation, too (y/n)? "));
			if (!get_yes_no(mathomatic)) {
				cl1_len = 0;
				cl1[cl1_len] = '\0';
			}
		}
		if (cl1_len) {
			snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("Does the plot command-line consist of any expressions (y/n)? "));
			if (get_yes_no(mathomatic)) {
				fprintf(mathomatic->gfp, _("Appending a comma to the command-line.\n"));
				if ((cl2_len + 2) >= sizeof(cl2)) {
					error(mathomatic, _("Command-line too long."));
					return false;
				}
				cl2[cl2_len++] = ',';
				cl2[cl2_len] = '\0';
			}
		}
	}
	if (strchr(cl2, 'y') || strchr(cl1, 'y')) {
		fprintf(mathomatic->gfp, _("Performing 3D surface plot...\n"));
#if	MINGW
		len = snprintf(cl, sizeof(cl), "gnuplot -persist -e \"%s; splot %s %s\"", mathomatic->plot_prefix, cl2, cl1);
#else
		len = snprintf(cl, sizeof(cl), "echo '%s; splot %s %s'|gnuplot -persist", mathomatic->plot_prefix, cl2, cl1);
#endif
	} else {
		fprintf(mathomatic->gfp, _("Performing 2D plot...\n"));
#if	MINGW
		len = snprintf(cl, sizeof(cl), "gnuplot -persist -e \"%s; plot %s %s\"", mathomatic->plot_prefix, cl2, cl1);
#else
		len = snprintf(cl, sizeof(cl), "echo '%s; plot %s %s'|gnuplot -persist", mathomatic->plot_prefix, cl2, cl1);
#endif
	}
	if (len >= sizeof(cl)) {
		error(mathomatic, _("gnuplot command-line too long."));
		return false;
	}
	ev = shell_out(mathomatic, cl);
	if (ev) {
		error(mathomatic, _("Possible error running gnuplot."));
		printf(_("Decimal exit value = %d\n"), ev);
		if (ev == 256 || ev == 1) {
			printf(_("Try separating each expression with a comma.\n"));
		}
		printf(_("Shell command-line = %s\n"), cl);
	}
	return true;
#undef APPEND
}
#endif

/*
 * The version command.
 *
 * All commands return true if successful.
 */
int
version_cmd(MathoMatic* mathomatic, char *cp)
//char	*cp;	/* the command-line argument */
{
	int	rv = true;		/* return value */
	int	status_flag = false;

	if (strncasecmp(cp, "status", 4) == 0) {
		status_flag = true;
		cp = skip_param(cp);
	}
	if (extra_characters(mathomatic, cp))	/* Make sure nothing else is on the command-line. */
		return false;
#if	LIBRARY
	free_result_str(mathomatic);
	mathomatic->result_str = strdup(VERSION);
#endif
	if (status_flag) {
		rv = version_report(mathomatic);
	} else {
#if	!SILENT || !LIBRARY
		fprintf(mathomatic->gfp, _("Mathomatic version %s\n"), VERSION);
#endif
	}

	if (status_flag) {
		debug_string(mathomatic, 0, "\nMathomatic is GNU LGPL version 2.1 licensed software,\n"
				"meaning it is free software that comes with no warranty.\n"
				"Type \"help license\" for the copyright and license.");

		EP(mathomatic, ("\nFor all new stuff, visit the Mathomatic website: www.mathomatic.org"));
	}
#if	LIBRARY
	return(rv && mathomatic->result_str != NULL);
#else
	return rv;
#endif
}

/*
 * Return the maximum amount of memory (in bytes) that this program will use.
 */
long
max_memory_usage(MathoMatic* mathomatic)
{
	return((long) (N_EQUATIONS + 3L) * (long) mathomatic->n_tokens * sizeof(token_type) * 2L);
}

/*
 * Try the function getrusage(2).
 *
 * Return true if successful.
 */
int
show_status(FILE *ofp)
{
#if	SHOW_RESOURCES
	struct rusage	usage_local;

	if (getrusage(RUSAGE_SELF, &usage_local) == 0) {
		fprintf(ofp, _("Total CPU usage, user time: %g seconds, system time: %g seconds.\n"),
		    (double) usage_local.ru_utime.tv_sec + ((double) usage_local.ru_utime.tv_usec / 1000000.0),
                    (double) usage_local.ru_stime.tv_sec + ((double) usage_local.ru_stime.tv_usec / 1000000.0));
		if (usage_local.ru_ixrss == 0 && usage_local.ru_idrss == 0 && usage_local.ru_isrss == 0) {
			if (usage_local.ru_maxrss)
				fprintf(ofp, _("Total RSS size: %ld kilobytes.\n"), usage_local.ru_maxrss);
		} else {
			fprintf(ofp, _("Total RSS size: %ld kbytes; shared text memory size: %ld kbytes*ticks;\n"), usage_local.ru_maxrss, usage_local.ru_ixrss);
			fprintf(ofp, _("Unshared data size: %ld kbytes*ticks; unshared stack size: %ld kbytes*ticks.\n"), usage_local.ru_idrss, usage_local.ru_isrss);
			fprintf(ofp, _("Number of times Mathomatic was swapped out: %ld; signals received: %ld.\n"), usage_local.ru_nswap, usage_local.ru_nsignals);
		}
		return true;
	}
#endif
	return false;
}

/*
 * Display version and status info.
 */
int
version_report(MathoMatic* mathomatic)
{
	long	l;

	fprintf(mathomatic->gfp, _("Mathomatic version %s\n"), VERSION);
	fprintf(mathomatic->gfp, _("The last main prompt return value is %d (meaning "), mathomatic->previous_return_value);
	switch (mathomatic->previous_return_value) {
	case 0:
		fprintf(mathomatic->gfp, _("failure).\n"));
		break;
	default:
		fprintf(mathomatic->gfp, _("success).\n"));
		break;
	}
	show_status(mathomatic->gfp);
	fprintf(mathomatic->gfp, _("\nCompile-time defines used: "));
#if	linux
	fprintf(mathomatic->gfp, "linux ");
#endif
#if	sun
	fprintf(mathomatic->gfp, "sun ");
#endif
#if	UNIX
	fprintf(mathomatic->gfp, "UNIX ");
#endif
#if	CYGWIN
	fprintf(mathomatic->gfp, "CYGWIN ");
#endif
#if	MINGW
	fprintf(mathomatic->gfp, "MINGW ");
#endif
#if	HANDHELD
	fprintf(mathomatic->gfp, "HANDHELD ");
#endif
#if	EDITLINE
	fprintf(mathomatic->gfp, "EDITLINE ");
#endif
#if	READLINE
	fprintf(mathomatic->gfp, "READLINE ");
#endif
#if	SILENT
	fprintf(mathomatic->gfp, "SILENT ");
#endif
#if	LIBRARY
	fprintf(mathomatic->gfp, "LIBRARY ");
#endif
#if	SECURE
	fprintf(mathomatic->gfp, "SECURE ");
#endif
#if	TIMEOUT_SECONDS
	fprintf(mathomatic->gfp, "TIMEOUT_SECONDS=%d ", TIMEOUT_SECONDS);
#endif
#if	I18N
	fprintf(mathomatic->gfp, "I18N ");
#endif
#if	NO_COLOR
	fprintf(mathomatic->gfp, "NO_COLOR ");
#endif
#if	BOLD_COLOR
	fprintf(mathomatic->gfp, "BOLD_COLOR ");
#endif
#if	WIN32_CONSOLE_COLORS
	fprintf(mathomatic->gfp, "WIN32_CONSOLE_COLORS ");
#endif
#if	NOGAMMA
	fprintf(mathomatic->gfp, "NOGAMMA ");
#endif
#if	USE_TGAMMA
	fprintf(mathomatic->gfp, "USE_TGAMMA ");
#endif
#if	DEBUG
	fprintf(mathomatic->gfp, "DEBUG ");
#endif
#if	VALGRIND
	fprintf(mathomatic->gfp, "VALGRIND ");
#endif
#if     SHOW_RESOURCES
	fprintf(mathomatic->gfp, "SHOW_RESOURCES ");
#endif

	fprintf(mathomatic->gfp, "\nsizeof(int) = %u bytes, sizeof(long) = %u bytes.\n", (unsigned) sizeof(int), (unsigned) sizeof(long));
	fprintf(mathomatic->gfp, "sizeof(double) = %u bytes, maximum double precision = %d decimal digits.\n", (unsigned) sizeof(double), DBL_DIG);
#ifdef	__VERSION__
#ifdef	__GNUC__
	fprintf(mathomatic->gfp, "GNU ");
#endif
	fprintf(mathomatic->gfp, _("C Compiler version: %s\n"), __VERSION__);
#endif

	fprintf(mathomatic->gfp, _("\n%d equation spaces currently allocated.\n"), mathomatic->n_equations);
	fprintf(mathomatic->gfp, _("The current expression array size is %d tokens,\n"), mathomatic->n_tokens);
	l = max_memory_usage(mathomatic) / 1000L;
	if (l >= 10000L) {
		fprintf(mathomatic->gfp, _("making the maximum memory usage approximately %ld megabytes.\n"), l / 1000L);
	} else {
		fprintf(mathomatic->gfp, _("making the maximum memory usage approximately %ld kilobytes.\n"), l);
	}
#if	SECURE
	fprintf(mathomatic->gfp, _("Compiled for maximum security.\n"));
#else
	fprintf(mathomatic->gfp, _("The current security level is %d"), mathomatic->security_level);
	switch (mathomatic->security_level) {
	case -1:
		fprintf(mathomatic->gfp, _(", meaning you are running m4 Mathomatic.\n"));
		break;
	case 0:
		fprintf(mathomatic->gfp, _(", no security, meaning users are unrestricted.\n"));
		break;
	case 1:
	case 2:
		fprintf(mathomatic->gfp, _(", some security.\n"));
		break;
	case 3:
		fprintf(mathomatic->gfp, _(", high security.\n"));
		break;
	case 4:
		fprintf(mathomatic->gfp, _(", maximum security.\n"));
		break;
	default:
		fprintf(mathomatic->gfp, _(", unknown meaning.\n"));
		break;
	}
#endif
#if	READLINE || EDITLINE
#if	READLINE
	fprintf(mathomatic->gfp, _("\nreadline is compiled in and "));
#else
	fprintf(mathomatic->gfp, _("\neditline is compiled in and "));
#endif
	if (mathomatic->readline_enabled) {
		fprintf(mathomatic->gfp, _("activated.\n"));
	} else {
		fprintf(mathomatic->gfp, _("deactivated.\n"));
	}
#elif	!LIBRARY && !HANDHELD
#if	MINGW
	SP(mathomatic, ("\nreadline is not compiled in, however some of its functionality"));
	SP(mathomatic, ("already exists in the Windows console for any"));
	EP(mathomatic, ("Windows console program (like Mathomatic)."));
#else
	SP(mathomatic, ("\nreadline is not compiled in."));
	SP(mathomatic, ("Please notify the package maintainer that readline"));
	EP(mathomatic, ("should be compiled into Mathomatic, with \"make READLINE=1\"."));
#endif
#endif
	return true;
}

/*
 * The solve command.
 *
 * Return 0 on solve failure for any equation which a solve was requested, or something was not verifiable,
 * with the "solve verifiable" option.
 * Return 1 on total success (all requested solves completed successfully),
 * or 2 if partial success (all solved, but some solutions didn't verify when doing "solve verify",
 * or the result contains infinity or NaN).
 */
int
solve_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i, j, k;
	int		start, stop;
	char		buf[MAX_CMD_LEN];
	int		diff_sign;
	int		verify_flag = 0, plural_flag, once_through, contains_infinity, did_something = false, last_solve_successful = false;
	char		*cp1, *cp_start;
	token_type	want;
	int		rv = 1;
	long		pre_v;		/* Mathomatic variable */

	cp_start = cp;
	if (strcmp_tospace(cp, "verify") == 0) {
		verify_flag = 1;
		cp = skip_param(cp);
	} else if (strcmp_tospace(cp, "verifiable") == 0) {
		verify_flag = 2;
		cp = skip_param(cp);
	}
	if (!get_range(mathomatic, &cp, &start, &stop)) {
		warning(mathomatic, _("No equations to solve."));
		return false;
	}
	i = next_espace(mathomatic);
	mathomatic->repeat_flag = true;
	if (strcmp_tospace(cp, "verify") == 0) {
		verify_flag = 1;
		cp = skip_param(cp);
	} else if (strcmp_tospace(cp, "verifiable") == 0) {
		verify_flag = 2;
		cp = skip_param(cp);
	}
	if (strcmp_tospace(cp, "for") == 0) {
		cp1 = skip_param(cp);
		if (*cp1) {
			cp = cp1;
		}
	}
	if (*cp == '\0') {
		my_strlcpy(mathomatic->prompt_str, _("Enter variable to solve for: "), sizeof(mathomatic->prompt_str));
		if ((cp = get_string(mathomatic, buf, sizeof(buf))) == NULL) {
			return false;
		}
		cp_start = cp;
	}
	mathomatic->input_column += (cp - cp_start);
	if ((cp = parse_equation(mathomatic, i, cp)) == NULL) {
		return false;
	}
	if (verify_flag) {
		if (mathomatic->n_lhs[i] != 1 || mathomatic->n_rhs[i] != 0 || (mathomatic->lhs[i][0].kind != VARIABLE
		    && (mathomatic->lhs[i][0].kind != CONSTANT || mathomatic->lhs[i][0].token.constant != 0.0))) {
			error(mathomatic, _("Can only verify for a single solve variable or identities after solving for 0."));
			goto fail;
		}
		want = mathomatic->lhs[i][0];
	}
	mathomatic->show_usage = false;
	for (k = start; k <= stop; k++) {
		if (k == i || mathomatic->n_lhs[k] <= 0 || mathomatic->n_rhs[k] <= 0) {
			continue;
		}
		last_solve_successful = false;
		mathomatic->cur_equation = k;

		did_something = true;
		if (verify_flag) {
			pre_v = 0;
			fprintf(mathomatic->gfp, _("Solving equation #%d for "), mathomatic->cur_equation + 1);
			list_proc(mathomatic, &want, 1, false);
			if (verify_flag == 2) {
				fprintf(mathomatic->gfp, " with required ");
			} else {
				fprintf(mathomatic->gfp, " with ");
			}
			if (want.kind == VARIABLE) {
				fprintf(mathomatic->gfp, "verification...\n");
				if (solved_equation(mathomatic, mathomatic->cur_equation)) {
					pre_v = mathomatic->lhs[mathomatic->cur_equation][0].token.variable;
				}
			} else {
				fprintf(mathomatic->gfp, "identity verification...\n");
			}
			copy_espace(mathomatic, mathomatic->cur_equation, i);
			if (solve_sub(mathomatic, &want, 1, mathomatic->lhs[mathomatic->cur_equation], &mathomatic->n_lhs[mathomatic->cur_equation], mathomatic->rhs[mathomatic->cur_equation], &mathomatic->n_rhs[mathomatic->cur_equation]) > 0) {
				simpa_repeat(mathomatic, mathomatic->cur_equation, true, false);	/* Solve result should be quick simplified. */
				last_solve_successful = true;
				debug_string(mathomatic, 0, _("Solve and \"repeat simplify quick\" successful:"));
				if (!return_result(mathomatic, mathomatic->cur_equation)) {		/* Display the simplified solve result. */
					goto fail;
				}
				if (want.kind == VARIABLE) {
					if (!solved_equation(mathomatic, mathomatic->cur_equation) || mathomatic->lhs[mathomatic->cur_equation][0].token.variable != want.token.variable) {
						error(mathomatic, _("Result not a properly solved equation, so cannot verify."));
						continue;
					}
					if (pre_v && pre_v == want.token.variable) {
						warning(mathomatic, _("Equation was already solved, so no need to verify solutions."));
						continue;
					}
				} else {
					copy_espace(mathomatic, mathomatic->cur_equation, i);
				}
				plural_flag = false;
				for (j = 0; j < mathomatic->n_rhs[mathomatic->cur_equation]; j += 2) {
					if (mathomatic->rhs[mathomatic->cur_equation][j].kind == VARIABLE && (mathomatic->rhs[mathomatic->cur_equation][j].token.variable & VAR_MASK) == SIGN) {
						plural_flag = true;
						break;
					}
				}
				if (want.kind == VARIABLE) {
					subst_var_with_exp(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->rhs[mathomatic->cur_equation], mathomatic->n_rhs[mathomatic->cur_equation], want.token.variable);
					subst_var_with_exp(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], mathomatic->rhs[mathomatic->cur_equation], mathomatic->n_rhs[mathomatic->cur_equation], want.token.variable);
				}
				once_through = false;
				calc_simp(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
				calc_simp(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
check_result:
				contains_infinity = (exp_contains_infinity(mathomatic->lhs[i], mathomatic->n_lhs[i])
				    || exp_contains_infinity(mathomatic->rhs[i], mathomatic->n_rhs[i]));
	                        if (se_compare(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i], mathomatic->rhs[i], mathomatic->n_rhs[i], &diff_sign) && (want.kind != VARIABLE || !diff_sign)) {
					if (want.kind != VARIABLE) {
						fprintf(mathomatic->gfp, _("This equation is an identity.\n"));
					} else if (plural_flag)
						fprintf(mathomatic->gfp, _("All solutions verified.\n"));
					else
						fprintf(mathomatic->gfp, _("Solution verified.\n"));
					if (contains_infinity) {
						error(mathomatic, _("Solution might be incorrect because it contains infinity or NaN."));
						if (rv)
							rv = 2;
					}
				} else {
					if (!contains_infinity && once_through < 2) {
						mathomatic->symb_flag = mathomatic->symblify;
						simpa_repeat(mathomatic, i, once_through ? false : true, once_through ? true : false);	/* Simplify to compare equation sides. */
						mathomatic->symb_flag = false;
						once_through++;
						goto check_result;
					}
					if (contains_infinity) {
						error(mathomatic, _("Solution might be incorrect because it contains infinity or NaN."));
					} else {
						if (want.kind != VARIABLE) {
							error(mathomatic, _("This equation is NOT an identity."));
						} else if (plural_flag)
							error(mathomatic, _("Unable to verify all solutions."));
						else
							error(mathomatic, _("Unable to verify solution."));
					}
					if (verify_flag == 2)
						rv = 0;
					else if (rv)
						rv = 2;
				}
			} else {
				printf(_("Solve failed for equation space #%d.\n"), mathomatic->cur_equation + 1);
				rv = 0;
			}
		} else {
			if (solve_espace(mathomatic, i, mathomatic->cur_equation)) {
				last_solve_successful = true;
				if (!return_result(mathomatic, mathomatic->cur_equation)) {
					goto fail;
				}
			} else {
				rv = 0;
			}
		}
	}
	if (did_something) {
		if (last_solve_successful && verify_flag) {
			debug_string(mathomatic, 1, "Verification identity:");
			list_esdebug(mathomatic, 1, i);
		}
	} else {
		printf(_("No work done.\n"));
	}
	mathomatic->n_lhs[i] = 0;
	mathomatic->n_rhs[i] = 0;
	return rv;
fail:
	mathomatic->n_lhs[i] = 0;
	mathomatic->n_rhs[i] = 0;
	return 0;
}

/*
 * The sum command.
 */
int
sum_cmd(MathoMatic* mathomatic, char *cp)
{
	return sum_product(mathomatic, cp, SUM_COMMAND);
}

/*
 * The product command.
 */
int
product_cmd(MathoMatic* mathomatic, char *cp)
{
	return sum_product(mathomatic, cp, PRODUCT_COMMAND);
}

/*
 * The for command.
 */
int
for_cmd(MathoMatic* mathomatic, char *cp)
{
	return sum_product(mathomatic, cp, FOR_COMMAND);
}

/*
 * Common function for the sum and product commands.
 */
static int
sum_product(MathoMatic* mathomatic, char *cp, enum spf_function current_function)
//char			*cp;		/* the command-line */
{
	int		i;
	long		v = 0;			/* Mathomatic variable */
	double		start, end, step = 1.0;
	int		result_equation;
	int		n, ns;
	token_type	*dest, *source;
	int		count_down;		/* if true, count down, otherwise count up */
	char		*cp1, buf[MAX_CMD_LEN];

	if (current_not_defined(mathomatic)) {
		return false;
	}
	result_equation = next_espace(mathomatic);
	if (mathomatic->n_rhs[mathomatic->cur_equation]) {
		ns = mathomatic->n_rhs[mathomatic->cur_equation];
		source = mathomatic->rhs[mathomatic->cur_equation];
		dest = mathomatic->rhs[result_equation];
	} else {
		ns = mathomatic->n_lhs[mathomatic->cur_equation];
		source = mathomatic->lhs[mathomatic->cur_equation];
		dest = mathomatic->lhs[result_equation];
	}
	if (*cp) {
		cp = parse_var2(mathomatic, &v, cp);
		if (cp == NULL) {
			return false;
		}
	}
	if (no_vars(source, ns, &v)) {
		error(mathomatic, _("Current expression contains no variables."));
		return false;
	}
	if (v == 0) {
		if (!prompt_var(mathomatic, &v)) {
			return false;
		}
	}
	if (!found_var(source, ns, v)) {
		error(mathomatic, _("Specified variable not found."));
		return false;
	}
	if (*cp) {
		if (*cp == '=') {
			cp++;
		}
		cp1 = cp;
	} else {
		list_var(mathomatic, v, 0);
		snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), "%s = ", mathomatic->var_str);
		if ((cp1 = get_string(mathomatic, buf, sizeof(buf))) == NULL)
			return false;
	}
	start = strtod(cp1, &cp);
	if (cp1 == cp || !isfinite(start)) {
		error(mathomatic, _("Number expected."));
		return false;
	}
	if (fabs(start) >= MAX_K_INTEGER) {
		error(mathomatic, _("Number too large."));
		return false;
	}
	cp = skip_comma_space(cp);
	if (strcmp_tospace(cp, "to") == 0) {
		cp = skip_param(cp);
	}
	if (*cp) {
		cp1 = cp;
	} else {
		my_strlcpy(mathomatic->prompt_str, _("To: "), sizeof(mathomatic->prompt_str));
		if ((cp1 = get_string(mathomatic, buf, sizeof(buf))) == NULL)
			return false;
	}
	end = strtod(cp1, &cp);
	if (cp1 == cp || !isfinite(end)) {
		error(mathomatic, _("Number expected."));
		return false;
	}
	if (fabs(end) >= MAX_K_INTEGER) {
		error(mathomatic, _("Number too large."));
		return false;
	}
	cp = skip_comma_space(cp);
	if (strcmp_tospace(cp, "step") == 0) {
		cp = skip_param(cp);
	}
	if (*cp) {
		cp1 = cp;
		step = fabs(strtod(cp1, &cp));
		if (cp1 == cp || !isfinite(step) || step <= 0.0 || step >= MAX_K_INTEGER) {
			error(mathomatic, _("Invalid step."));
			return false;
		}
	}
	if (extra_characters(mathomatic, cp))
		return false;
	count_down = (end < start);
	if (fmod(fabs(start - end) / step, 1.0) != 0.0) {
		warning(mathomatic, _("End value not reached."));
	}
	if (current_function == PRODUCT_COMMAND) {
		dest[0] = mathomatic->one_token;
	} else {
		dest[0] = mathomatic->zero_token;
	}
	n = 1;
	for (; count_down ? (start >= end) : (start <= end); count_down ? (start -= step) : (start += step)) {
		if (n + 1 + ns > mathomatic->n_tokens) {
			error_huge(mathomatic);
		}
		blt(mathomatic->tlhs, source, ns * sizeof(token_type));
		mathomatic->n_tlhs = ns;
		for (i = 0; i < mathomatic->n_tlhs; i += 2) {
			if (mathomatic->tlhs[i].kind == VARIABLE && mathomatic->tlhs[i].token.variable == v) {
				mathomatic->tlhs[i].kind = CONSTANT;
				mathomatic->tlhs[i].token.constant = start;
			}
		}
		if (current_function != FOR_COMMAND) {
			for (i = 0; i < mathomatic->n_tlhs; i++) {
				mathomatic->tlhs[i].level++;
			}
			for (i = 0; i < n; i++) {
				dest[i].level++;
			}
			dest[n].kind = OPERATOR;
			dest[n].level = 1;
		}
		switch (current_function) {
		case PRODUCT_COMMAND:
			dest[n].token.operatr = TIMES;
			n++;
			break;
		case SUM_COMMAND:
			dest[n].token.operatr = PLUS;
			n++;
			break;
		case FOR_COMMAND:
			n = 0;
			break;
		}
		blt(&dest[n], mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
		n += mathomatic->n_tlhs;
		calc_simp(mathomatic, dest, &n);
		if (current_function == FOR_COMMAND) {
			list_var(mathomatic, v, 0);
			fprintf(mathomatic->gfp, "%s = %.*g: ", mathomatic->var_str, mathomatic->precision, start);
			list_factor(mathomatic, dest, &n, false);
			fprintf(mathomatic->gfp, "\n");
		} else {
			side_debug(mathomatic, 1, dest, n);
		}
	}
	if (current_function == FOR_COMMAND) {
		return true;
	} else {
		if (mathomatic->n_rhs[mathomatic->cur_equation]) {
			mathomatic->n_rhs[result_equation] = n;
			blt(mathomatic->lhs[result_equation], mathomatic->lhs[mathomatic->cur_equation], mathomatic->n_lhs[mathomatic->cur_equation] * sizeof(token_type));
			mathomatic->n_lhs[result_equation] = mathomatic->n_lhs[mathomatic->cur_equation];
		} else {
			mathomatic->n_lhs[result_equation] = n;
		}
		return return_result(mathomatic, result_equation);
	}
}

/*
 * This function is for the "optimize" command.
 * It finds and substitutes all occurrences of the RHS of "en" in "equation".
 * It should be called repeatedly until it returns false.
 */
static int
find_more(MathoMatic* mathomatic, token_type *equation, int *np, int en)
//token_type	*equation;	/* expression to search */
//int		*np;		/* pointer to length of expression */
//int		en;		/* equation space number */
{
	int	i, j, k;
	int	level;
	int	diff_sign;
	int	found_se;	/* found sub-expression flag */

	if (*np <= 0 || !solved_equation(mathomatic, en)) {
		return false;
	}
	for (level = 1, found_se = true; found_se; level++) {
		for (i = 1, found_se = false; i < *np; i = j + 2) {
			for (j = i; j < *np && equation[j].level >= level; j += 2)
				;
			if (j == i) {
				continue;
			}
			found_se = true;
			k = i - 1;
			if (se_compare(mathomatic, &equation[k], j - k, mathomatic->rhs[en], mathomatic->n_rhs[en], &diff_sign)) {
				if (diff_sign) {
					blt(&equation[i+2], &equation[j], (*np - j) * sizeof(token_type));
					*np -= (j - (i + 2));
					level++;
					equation[k].level = level;
					equation[k].kind = CONSTANT;
					equation[k].token.constant = -1.0;
					k++;
					equation[k].level = level;
					equation[k].kind = OPERATOR;
					equation[k].token.operatr = TIMES;
					k++;
				} else {
					blt(&equation[i], &equation[j], (*np - j) * sizeof(token_type));
					*np -= (j - i);
				}
				equation[k].level = level;
				equation[k].kind = VARIABLE;
				equation[k].token.variable = mathomatic->lhs[en][0].token.variable;
				return true;
			}
		}
	}
	return false;
}

/*
 * This function is for the "optimize" command.
 * It finds and replaces all repeated expressions in "equation" with temporary variables.
 * It also creates a new equation for each temporary variable.
 * It should be called repeatedly until it returns false.
 */
static int
opt_es(MathoMatic* mathomatic, token_type *equation, int *np)
{
	int	i, j, k, i1, i2, jj1, k1;
	int	level, level1;
	int	diff_sign;
	int	found_se, found_se1;	/* found sub-expression flags */
	long	v;			/* Mathomatic variable */
	char	var_name_buf[MAX_VAR_LEN];

	if (*np <= 0) {
		return false;
	}
	for (level = 1, found_se = true; found_se; level++) {
		for (i = 1, found_se = false; i < *np; i = j + 2) {
			for (j = i; j < *np && equation[j].level > level; j += 2)
				;
			if (j == i) {
				continue;
			}
			found_se = true;
			k = i - 1;
			if ((j - k) < OPT_MIN_SIZE) {
				continue;
			}
			found_se1 = true;
			for (level1 = 1; found_se1; level1++) {
				for (i1 = 1, found_se1 = false; i1 < *np; i1 = jj1 + 2) {
					for (jj1 = i1; jj1 < *np && equation[jj1].level > level1; jj1 += 2) {
					}
					if (jj1 == i1) {
						continue;
					}
					found_se1 = true;
					if (i1 <= j)
						continue;
					k1 = i1 - 1;
					if ((jj1 - k1) >= OPT_MIN_SIZE
					    && se_compare(mathomatic, &equation[k], j - k, &equation[k1], jj1 - k1, &diff_sign)) {
						snprintf(var_name_buf, sizeof(var_name_buf), "temp%.0d", mathomatic->last_temp_var);
						if (parse_var(mathomatic, &v, var_name_buf) == NULL) {
							return false;	/* can't create "temp" variable */
						}
						mathomatic->last_temp_var++;
						if (mathomatic->last_temp_var < 0) {
							mathomatic->last_temp_var = 0;
						}
						i2 = next_espace(mathomatic);
						mathomatic->lhs[i2][0].level = 1;
						mathomatic->lhs[i2][0].kind = VARIABLE;
						mathomatic->lhs[i2][0].token.variable = v;
						mathomatic->n_lhs[i2] = 1;
						blt(mathomatic->rhs[i2], &equation[k], (j - k) * sizeof(token_type));
						mathomatic->n_rhs[i2] = j - k;
						if (diff_sign) {
							blt(&equation[i1+2], &equation[jj1], (*np - jj1) * sizeof(token_type));
							*np -= (jj1 - (i1 + 2));
							level1++;
							equation[k1].level = level1;
							equation[k1].kind = CONSTANT;
							equation[k1].token.constant = -1.0;
							k1++;
							equation[k1].level = level1;
							equation[k1].kind = OPERATOR;
							equation[k1].token.operatr = TIMES;
							k1++;
						} else {
							blt(&equation[i1], &equation[jj1], (*np - jj1) * sizeof(token_type));
							*np -= (jj1 - i1);
						}
						equation[k1].level = level1;
						equation[k1].kind = VARIABLE;
						equation[k1].token.variable = v;
						blt(&equation[i], &equation[j], (*np - j) * sizeof(token_type));
						*np -= j - i;
						equation[k].level = level;
						equation[k].kind = VARIABLE;
						equation[k].token.variable = v;
						while (find_more(mathomatic, equation, np, i2))
							;
						simp_loop(mathomatic, mathomatic->rhs[i2], &mathomatic->n_rhs[i2]);
						simp_loop(mathomatic, equation, np);
						for (i = 0;; i++) {
							if (i >= N_EQUATIONS) {
								error_bug(mathomatic, "Too many optimized equations.");
							}
							if (mathomatic->opt_en[i] < 0)
								break;
						}
						mathomatic->opt_en[i] = i2;
						mathomatic->opt_en[i+1] = -1;
						return true;
					}
				}
			}
		}
	}
	return false;
}

/*
 * The optimize command.
 */
int
optimize_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, j, k, i1;
	int	start, stop;
	int	rv = false, flag, skip_flag;
	int	start_en;
	int	diff_sign;

	if (!get_range_eol(mathomatic, &cp, &start, &stop)) {
		return false;
	}
	mathomatic->opt_en[0] = -1;
	start_en = 0;
	for (j = i = start; i <= stop; i++) {
		if (mathomatic->n_lhs[i]) {
			j = i;
			simp_equation(mathomatic, i);
		}
	}
	stop = j;
	do {
		flag = false;
		for (i = start; i <= stop; i++) {
			for (j = start; j <= stop; j++) {
				if (i != j) {
					while (find_more(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], j)) {
						flag = true;
						rv = true;
					}
				}
			}
		}
	} while (flag);
	for (i = start; i <= stop; i++) {
		if (mathomatic->n_lhs[i] == 0)
			continue;
		do {
			flag = false;
			simp_equation(mathomatic, i);
			for (j = 0; mathomatic->opt_en[j] >= 0; j++) {
				if (i != mathomatic->opt_en[j]) {
					simp_equation(mathomatic, mathomatic->opt_en[j]);
					while (find_more(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->opt_en[j]))
						flag = true;
					while (find_more(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], mathomatic->opt_en[j]))
						flag = true;
				}
			}
		} while (flag);
		while (opt_es(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i])) {
			rv = true;
		}
		while (opt_es(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i])) {
			rv = true;
		}
		if (rv) {
			for (i1 = start_en; mathomatic->opt_en[i1] >= 0; i1++) {
				for (j = start_en; mathomatic->opt_en[j] >= 0; j++) {
					for (k = j + 1; mathomatic->opt_en[k] >= 0; k++) {
						while (find_more(mathomatic, mathomatic->rhs[mathomatic->opt_en[k]], &mathomatic->n_rhs[mathomatic->opt_en[k]], mathomatic->opt_en[j]))
							;
						while (find_more(mathomatic, mathomatic->rhs[mathomatic->opt_en[j]], &mathomatic->n_rhs[mathomatic->opt_en[j]], mathomatic->opt_en[k]))
							;
					}
				}
				while (opt_es(mathomatic, mathomatic->rhs[mathomatic->opt_en[i1]], &mathomatic->n_rhs[mathomatic->opt_en[i1]]))
					;
			}
			/* Remove equation if identity, otherwise display. */
			for (; mathomatic->opt_en[start_en] >= 0; start_en++) {
				k = mathomatic->opt_en[start_en];
				if (se_compare(mathomatic, mathomatic->lhs[k], mathomatic->n_lhs[k], mathomatic->rhs[k], mathomatic->n_rhs[k], &diff_sign) && !diff_sign) {
					mathomatic->n_lhs[k] = 0;
					mathomatic->n_rhs[k] = 0;
				} else
					list_sub(mathomatic, k);
			}
			if (se_compare(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i], mathomatic->rhs[i], mathomatic->n_rhs[i], &diff_sign) && !diff_sign) {
				mathomatic->n_lhs[i] = 0;
				mathomatic->n_rhs[i] = 0;
			}
		}
	}
	if (rv) {
		for (i = start; i <= stop; i++) {
			if (mathomatic->n_lhs[i] == 0)
				continue;
			skip_flag = false;
			do {
				flag = false;
				simp_equation(mathomatic, i);
				for (j = 0; mathomatic->opt_en[j] >= 0; j++) {
					if (i != mathomatic->opt_en[j]) {
						simp_equation(mathomatic, mathomatic->opt_en[j]);
						while (find_more(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->opt_en[j]))
							flag = true;
						while (find_more(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], mathomatic->opt_en[j]))
							flag = true;
					} else
						skip_flag = true;
				}
			} while (flag);
			if (!skip_flag)
				list_sub(mathomatic, i);
		}
	}
	if (!rv) {
		error(mathomatic, _("Unable to find any repeated expressions."));
	}
	return rv;
}

#if	READLINE || EDITLINE
/*
 * The push command.
 */
int
push_cmd(MathoMatic* mathomatic, char *cp)
{
	int	start, stop;
	int	k;
	char	*cp1, *cp_start;

	cp_start = cp;
	if (!mathomatic->readline_enabled) {
		error(mathomatic, _("Readline is currently turned off."));
		return false;
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &start, &stop)) {
			if (*cp_start) {
				reset_error(mathomatic);
			}
			goto push_text;
		}
		if (*cp && cp == cp1) {
			goto push_text;
		}
		for (k = start; k <= stop; k++) {
			if (mathomatic->n_lhs[k]) {
				if (push_en(mathomatic, k)) {
					debug_string(mathomatic, 0, _("Expression pushed.  Press the UP key to access."));
				} else {
					error(mathomatic, _("Expression push failed."));
					return false;
				}
			}
		}
	} while (*cp);
	return true;

push_text:
	if (*cp_start) {
		add_history(cp_start);
		mathomatic->last_history_string = NULL;
		debug_string(mathomatic, 0, _("Text string pushed.  Press the UP key to access."));
		return true;
	}
	return false;
}

/*
 * Push an equation space into the readline history.
 *
 * Return true if successful.
 */
int
push_en(MathoMatic* mathomatic, int en)
//int	en;	/* equation space number to push */
{
	char	*cp;

	if (!mathomatic->readline_enabled)
		return false;
	mathomatic->high_prec = true;
	cp = list_equation(mathomatic, en, false);
	mathomatic->high_prec = false;
	if (cp == NULL)
		return false;
	add_history(cp);
	mathomatic->last_history_string = cp;
	return true;
}
#endif

/*
 * Output the current working directory.
 *
 * Return true if successful.
 */
int
output_current_directory(MathoMatic* mathomatic, FILE *ofp)
//FILE	*ofp;	/* output file pointer */
{
#if	!SECURE
	char	buf[MAX_CMD_LEN];

	if (mathomatic->security_level < 3 && ofp) {
		if (getcwd(buf, sizeof(buf))) {
			fprintf(ofp, "directory %s\n", buf);
			return true;
		} else {
			perror(NULL);
		}
	}
#endif
	return false;
}

int
fprintf_escaped(FILE *ofp, char *cp)
{
	int	len = 0;

	while (*cp) {
		if (*cp == ';') {
			len += fprintf(ofp, "\\");
		}
		len += fprintf(ofp, "%c", *cp);
		cp++;
	}
	return len;
}

/*
 * Output the current set options in a format suitable for reading back in.
 * If all_set_options, include options you don't want to save.
 */
void
output_options(MathoMatic* mathomatic, FILE *ofp, int all_set_options)
//FILE	*ofp;	/* output file pointer */
{
	if (ofp == NULL)
		return;

	fprintf(ofp, "precision = %d digits\n", mathomatic->precision);

	if (!mathomatic->autosolve) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "autosolve\n");

	if (!mathomatic->autocalc) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "autocalc\n");

	if (!mathomatic->autodelete) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "autodelete\n");

	if (!mathomatic->autoselect) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "autoselect\n");

#if	!SILENT
	fprintf(ofp, "debug_level = %d\n", mathomatic->debug_level);
#endif

	if (!mathomatic->case_sensitive_flag) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "case_sensitive\n");

	if (all_set_options && mathomatic->html_flag) {
		if (mathomatic->html_flag == 2) {
			fprintf(ofp, "all html ");
		} else {
			fprintf(ofp, "html ");
		}
	}
	if (mathomatic->color_flag == 2) {
		fprintf(ofp, "alternative ");
	}
	if (mathomatic->bold_colors && mathomatic->color_flag) {
		fprintf(ofp, "bold color");
	} else {
		if (!mathomatic->color_flag) {
			fprintf(ofp, "no color");
		} else {
			fprintf(ofp, "no bold color");
		}
	}
	if (mathomatic->text_color >= 0) {
		fprintf(ofp, " %d", mathomatic->text_color);
	}
	fprintf(ofp, "\n");

	if (!mathomatic->display2d) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "display2d\n");

	if (all_set_options) {
		fprintf(ofp, "columns = %d, ", mathomatic->screen_columns);
		fprintf(ofp, "rows = %d\n", mathomatic->screen_rows);
	}

	fprintf(ofp, "fractions_display_mode = ");
	switch (mathomatic->fractions_display) {
	case 0:
		fprintf(ofp, "none\n");
		break;
	case 2:
		fprintf(ofp, "mixed\n");
		break;
	default:
		fprintf(ofp, "simple\n");
		break;
	}

	if (mathomatic->quiet_mode) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "prompt\n");

#if	0
	if (!mathomatic->preserve_surds) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "preserve_surds\n");
#endif

	if (!mathomatic->rationalize_denominators) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "rationalize_denominators\n");

	fprintf(ofp, "modulus_mode = ");
	switch (mathomatic->modulus_mode) {
	case 0:
		fprintf(ofp, "C\n");
		break;
	case 1:
		fprintf(ofp, "Python\n");
		break;
	case 2:
		fprintf(ofp, "normal\n");
		break;
	default:
		fprintf(ofp, "unknown\n");
		break;
	}

	if (mathomatic->finance_option < 0) {
		fprintf(ofp, "no fixed_point\n");
	} else {
		fprintf(ofp, "fixed_point = %d\n", mathomatic->finance_option);
	}

	if (!mathomatic->factor_int_flag) {
		fprintf(ofp, "no ");
	}
	fprintf(ofp, "factor_integers\n");

	if (mathomatic->right_associative_power) {	/* option is hardly ever used */
		fprintf(ofp, "right_associative_power\n");
	}

#if	SHELL_OUT
	fprintf(ofp, "plot_prefix = ");
	fprintf_escaped(ofp, mathomatic->plot_prefix);
	fprintf(ofp, "\n");
#endif

	fprintf(ofp, "special_variable_characters = %s\n", mathomatic->special_variable_characters);
}

/*
 * Skip over a yes/no indicator and return true if *cpp pointed to a negative word.
 */
int
skip_no(char **cpp)
{
	if (strcmp_tospace(*cpp, "no") == 0
	    || strcmp_tospace(*cpp, "not") == 0
	    || strcmp_tospace(*cpp, "off") == 0
	    || strcmp_tospace(*cpp, "false") == 0) {
		*cpp = skip_param(*cpp);
		return true;
	}
	if (strcmp_tospace(*cpp, "yes") == 0
	    || strcmp_tospace(*cpp, "on") == 0
	    || strcmp_tospace(*cpp, "true") == 0) {
		*cpp = skip_param(*cpp);
	}
	return false;
}

#if	!SECURE
/*
 * Save set options in the startup file, displaying a confirmation message.
 * If a string is passed, then just save the string.
 *
 * Return true if successful.
 */
int
save_set_options(MathoMatic* mathomatic, char *cp)
{
	FILE	*fp;
	int	pre_existing;

	if (mathomatic->rc_file[0] == '\0') {
		error(mathomatic, _("Set options startup file name not set; contact the developer."));
		return false;
	}
	pre_existing = (access(mathomatic->rc_file, F_OK) == 0);
	if ((fp = fopen(mathomatic->rc_file, "w")) == NULL) {
		perror(mathomatic->rc_file);
		error(mathomatic, _("Unable to write to set options startup file."));
		return false;
	}
	fprintf(fp, "; Mathomatic set options loaded at startup,\n");
	fprintf(fp, "; created by the \"set save\" command.\n");
	fprintf(fp, "; This file can be edited or deleted.\n\n");
	if (cp && *cp) {
		fprintf(fp, "%s\n", cp);
	} else {
		output_options(mathomatic, fp, false);
	}
	if (fclose(fp) == 0) {
		if (pre_existing)
			printf(_("Startup file \"%s\" overwritten with set options.\n"), mathomatic->rc_file);
		else
			printf(_("Set options saved in startup file \"%s\".\n"), mathomatic->rc_file);
	} else {
		perror(mathomatic->rc_file);
		error(mathomatic, _("Error saving set options."));
		return false;
	}
	return true;
}
#endif

/*
 * Handle parsing of options for the set command.
 *
 * Return false if error.
 */
int
set_options(MathoMatic* mathomatic, char *cp, int loading_startup_file)
{
	int	i;
	int	negate;
	char	*cp1 = NULL, *option_string;

	mathomatic->show_usage = false;	/* this command gives enough usage information */
try_next_param:
	cp = skip_comma_space(cp);
	if (*cp == '\0') {
		return true;
	}
	if (strncasecmp(cp, "directory", 3) == 0) {
		cp = skip_param(cp);
#if	!SECURE
		if (mathomatic->security_level < 3) {
	 		if (*cp == '\0') {
				cp1 = getenv("HOME");
				if (cp1 == NULL) {
					error(mathomatic, _("HOME environment variable not set."));
					return false;
				}
				cp = cp1;
			}
			if (chdir(cp)) {
				perror(cp);
				error(mathomatic, _("Error changing directory."));
				return false;
			}
			printf(_("Current working directory changed to "));
			return output_current_directory(mathomatic, stdout);
		}
#endif
		error(mathomatic, _("Option disabled by security level."));
		return false;
	}
	negate = skip_no(&cp);
	option_string = cp;
	cp = skip_param(cp);
#if	!SILENT
	if (strncasecmp(option_string, "debug", 5) == 0) {
		if (negate) {
			mathomatic->debug_level = 0;
		} else {
			i = decstrtol(cp, &cp1);
			if (cp1 == NULL || cp == cp1) {
				error(mathomatic, _("Please specify the debug level number from -2 to 6."));
				return false;
			}
			cp = cp1;
			mathomatic->debug_level = i;
		}
		goto try_next_param;
	}
#endif
	if (strncasecmp(option_string, "special", 7) == 0) {
		if (negate) {
			mathomatic->special_variable_characters[0] = '\0';
		} else {
			for (i = 0; cp[i]; i++) {
				if (is_mathomatic_operator(cp[i])) {
					error(mathomatic, _("Invalid character in list, character is a Mathomatic operator."));
					return false;
				}
			}
			my_strlcpy(mathomatic->special_variable_characters, cp, sizeof(mathomatic->special_variable_characters));
		}
		return true;
	}
#if	SHELL_OUT
	if (strncasecmp(option_string, "plot_prefix", 4) == 0) {
		if (negate) {
			mathomatic->plot_prefix[0] = '\0';
		} else {
			my_strlcpy(mathomatic->plot_prefix, cp, sizeof(mathomatic->plot_prefix));
		}
		return true;
	}
#endif
	if (strncasecmp(option_string, "rows", 3) == 0) {
		if (negate) {
			mathomatic->screen_rows = 0;
		} else {
			if (*cp == '\0') {
				printf(_("Current screen rows is %d.\n"), mathomatic->screen_rows);
				goto check_return;
			}
			i = decstrtol(cp, &cp1);
			if (i < 0 || cp1 == NULL || cp == cp1) {
				error(mathomatic, _("Please specify how tall the screen is; 0 = no pagination."));
				return false;
			}
			cp = cp1;
			mathomatic->screen_rows = i;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "columns", 6) == 0) {
		if (negate) {
			mathomatic->screen_columns = 0;
		} else {
			if (*cp == '\0') {
				if (!get_screen_size(mathomatic)) {
					error(mathomatic, _("OS failed to return screen size."));
					return false;
				}
				goto check_return;
			}
			i = decstrtol(cp, &cp1);
			if (i < 0 || cp1 == NULL || cp == cp1) {
				error(mathomatic, _("Please specify how wide the screen is; 0 = no limit."));
				return false;
			}
			cp = cp1;
			mathomatic->screen_columns = i;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "wide", 4) == 0) {
		if (negate) {
			if (!get_screen_size(mathomatic) || mathomatic->screen_columns == 0) {
				error(mathomatic, _("OS failed to return screen size."));
				return false;
			}
		} else {
			mathomatic->screen_columns = 0;
			mathomatic->screen_rows = 0;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "precision", 4) == 0) {
		i = decstrtol(cp, &cp1);
		if (i < 0 || i > 15 || cp1 == NULL || cp == cp1) {
			error(mathomatic, _("Please specify a display precision between 0 and 15 digits."));
			return false;
		}
		mathomatic->precision = i;
		return true;
	}
	if (strcmp_tospace(option_string, "auto") == 0) {
		mathomatic->autosolve = mathomatic->autocalc = mathomatic->autoselect = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "autosolve", 9) == 0) {
		mathomatic->autosolve = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "autocalc", 8) == 0) {
		mathomatic->autocalc = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "autodelete", 7) == 0) {
		mathomatic->autodelete = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "autoselect", 10) == 0) {
		mathomatic->autoselect = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "case", 4) == 0) {
		mathomatic->case_sensitive_flag = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "display2d", 7) == 0) {
		mathomatic->display2d = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "fractions", 4) == 0) {
		if (negate) {
			mathomatic->fractions_display = 0;
		} else {
			i = decstrtol(cp, &cp1);
			if (cp == cp1) {
				if (strcmp_tospace(cp, "none") == 0) {
					cp1 = skip_param(cp);
					i = 0;
				} else if (strcmp_tospace(cp, "simple") == 0) {
					cp1 = skip_param(cp);
					i = 1;
				} else if (strcmp_tospace(cp, "mixed") == 0) {
					cp1 = skip_param(cp);
					i = 2;
				}
			}
			if (cp1 == NULL || cp == cp1 || i < 0 || i > 2) {
				error(mathomatic, _("Please specify the fractions display mode number (0, 1, or 2)."));
				printf(_("0 means do not display any constants as fractions,\n"));
				printf(_("1 means display some constants as \"simple\" fractions,\n"));
                		printf(_("2 means display some constants as \"mixed\" or simple fractions.\n"));
				printf(_("Current value is %d.\n"), mathomatic->fractions_display);
				return false;
			}
			cp = cp1;
			mathomatic->fractions_display = i;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "prompt", 6) == 0) {
		mathomatic->quiet_mode = negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "demo", 4) == 0) {
		mathomatic->demo_mode = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "html", 4) == 0) {
#if	!SECURE
		if (mathomatic->security_level > 0) {
#endif
			error(mathomatic, _("Option disabled by security level."));
			return false;
#if	!SECURE
		}
#endif
		reset_attr(mathomatic);
		if (is_all(cp)) {
			cp = skip_param(cp);
			if (negate)
				mathomatic->html_flag = 0;
			else
				mathomatic->html_flag = 2;
		} else {
			mathomatic->html_flag = !negate;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "preserve_surds", 13) == 0) {
		mathomatic->preserve_surds = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "rationalize", 11) == 0) {
		mathomatic->rationalize_denominators = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "modulus_mode", 3) == 0) {
		if (negate) {
			error(mathomatic, _("Modulus mode cannot be turned off."));
			return false;
		} else {
			i = decstrtol(cp, &cp1);
			if (cp == cp1) {
				if (strcmp_tospace(cp, "C") == 0 || strcmp_tospace(cp, "java") == 0) {
					cp1 = skip_param(cp);
					i = 0;
				} else if (strcmp_tospace(cp, "python") == 0) {
					cp1 = skip_param(cp);
					i = 1;
				} else if (strcmp_tospace(cp, "positive") == 0 || strcmp_tospace(cp, "normal") == 0) {
					cp1 = skip_param(cp);
					i = 2;
				}
			}
			if (cp1 == NULL || cp == cp1 || i < 0 || i > 2) {
				error(mathomatic, _("Please specify the modulus mode number (0, 1, or 2)."));
				printf(_("* \"C\" and \"Java\" programming language mode 0:\n"));
				printf(_("  0 means modulus operator (dividend %% divisor) result has same sign as dividend;\n"));
				printf(_("* \"Python\" programming language mode 1:\n"));
				printf(_("  1 means computed result always has same sign as the divisor;\n"));
				printf(_("* Mathematically correct mode 2 for perfect simplification:\n"));
                		printf(_("  2 means the result is always \"positive\" or zero (\"normal\" mode).\n\n"));
				printf(_("The current value is %d ("), mathomatic->modulus_mode);
				switch (mathomatic->modulus_mode) {
				case 0:
					printf("C");
					break;
				case 1:
					printf("Python");
					break;
				case 2:
					printf("normal");
					break;
				default:
					printf("unknown");
					break;
				}
				printf(_(" mode).\n"));
				return false;
			}
			cp = cp1;
			mathomatic->modulus_mode = i;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "color", 5) == 0) {
		reset_attr(mathomatic);
		if (mathomatic->color_flag != 2 || negate) {
			mathomatic->color_flag = !negate;
		}
		i = decstrtol(cp, &cp1);
		if (cp1 && cp != cp1) {
			mathomatic->text_color = i;
			cp = cp1;
		} else {
			mathomatic->text_color = -1;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "alternative", 3) == 0) {
		reset_attr(mathomatic);
		mathomatic->color_flag = (!negate) + 1;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "bold", 4) == 0) {
		reset_attr(mathomatic);
		mathomatic->bold_colors = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "fixed", 3) == 0) {
		if (negate) {
			mathomatic->finance_option = -1;
		} else {
			i = decstrtol(cp, &cp1);
			if (cp1 == NULL) {
				return false;
			}
			if (cp == cp1) {
				if (*cp1 == '\0') {
					i = 2;
				} else {
					error(mathomatic, _("Please specify the number of digits to display after the decimal point."));
					return false;
				}
			}
			if (i < -1 || i > 100) {
				error(mathomatic, _("Range is -1 to 100; Sets rounded display with fixed number of trailing digits."));
				return false;
			}
			if (i == 0) {
				warning(mathomatic, "Setting rounded, integer-only display.");
			}
			cp = cp1;
			mathomatic->finance_option = i;
		}
		goto try_next_param;
	}
	if (strncasecmp(option_string, "factor_integers", 6) == 0) {
		mathomatic->factor_int_flag = !negate;
		goto try_next_param;
	}
	if (strncasecmp(option_string, "right_associative_power", 5) == 0) {
		mathomatic->right_associative_power = !negate;
		goto try_next_param;
	}
	if (strcmp_tospace(option_string, "load") == 0) {
#if	!SECURE
		if (negate) {
			printf(_("Doing nothing.\n"));
			return true;
		}
		if (loading_startup_file) {
			printf(_("Ignoring recursive \"set load\".\n"));
			return true;
		}
		if (extra_characters(mathomatic, cp))
			return false;
		if (mathomatic->security_level <= 3) {
			if (load_rc(mathomatic, false, mathomatic->gfp)) {
				fprintf(mathomatic->gfp, _("\nEnd of file.\n"));
				return true;
			} else {
				error(mathomatic, _("Error loading startup set options."));
				return false;
			}
		}
#endif
		error(mathomatic, _("Option disabled by security level."));
		return false;
	}
	if (strcmp_tospace(option_string, "save") == 0) {
#if	!SECURE
		if (mathomatic->security_level < 2) {
			if (mathomatic->rc_file[0] == '\0') {
				error(mathomatic, _("Set options startup file name not set; contact the developer."));
				return false;
			}
			if (loading_startup_file) {
				printf(_("Got \"set save\" while loading startup options, quitting.\n"));
				return false;
			}
			if (negate) {
				if (extra_characters(mathomatic, cp))
					return false;
				if (unlink(mathomatic->rc_file) == 0) {
					printf(_("Set options startup file \"%s\" removed.\n"), mathomatic->rc_file);
					printf(_("Factory default options will be used on next startup of Mathomatic.\n"));
					return true;
				} else {
					perror(mathomatic->rc_file);
					error(mathomatic, _("Set options startup file cannot be removed."));
					return false;
				}
			} else {
				if (save_set_options(mathomatic, cp)) {
					if (load_rc(mathomatic, false, mathomatic->gfp)) {
						fprintf(mathomatic->gfp, _("\nNew startup set options loaded.\n"));
						return true;
					} else {
						error(mathomatic, _("Error loading new startup set options."));
						fprintf(mathomatic->gfp, _("Correct or type \"set no save\" to remove.\n"));
					}
				}
				return false;
			}
		}
#endif
		error(mathomatic, _("Option disabled by security level."));
		return false;
	}
	if (strcmp_tospace(option_string, "set") == 0) {
		if (!negate)
			goto try_next_param;
	}
	printf(_("\nCannot process set string \"%s\".\n"), option_string);
	error(mathomatic, _("Unknown set option."));
	return false;

check_return:
	extra_characters(mathomatic, cp);
	return true;
}

/*
 * The set command.
 */
int
set_cmd(MathoMatic* mathomatic, char *cp)
{
	int	rv;

	if (*cp == '\0') {
		fprintf(mathomatic->gfp, _("Options are set as follows:\n\n"));

		output_options(mathomatic, mathomatic->gfp, true);

		output_current_directory(mathomatic, mathomatic->gfp);
		return true;
	}
	rv = set_options(mathomatic, cp, false);
	if (rv) {
		debug_string(mathomatic, 0, _("Success."));
	}
	return rv;
}

/*
 * The echo command.
 */
int
echo_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i;
	int	len = 0;
	int	width, height;

	if (mathomatic->repeat_flag) {
		if (*cp) {
			if (mathomatic->screen_columns)
				width = mathomatic->screen_columns;
			else
				width = TEXT_COLUMNS;
			while ((len + strlen(cp)) < width) {
				fprintf(mathomatic->gfp, "%s", cp);
				len += strlen(cp);
			}
			fprintf(mathomatic->gfp, "\n");
		} else {
			if (mathomatic->screen_rows)
				height = mathomatic->screen_rows;
			else
				height = TEXT_ROWS;
			for (i = 0; i < height; i++) {
				fprintf(mathomatic->gfp, "\n");
			}
		}
	} else {
		fprintf(mathomatic->gfp, "%s\n", cp);
	}
	return true;
}

/*
 * The pause command.
 */
int
pause_cmd(MathoMatic* mathomatic, char *cp)
{
#if	LIBRARY
	return true;
#else
	char	*cp1;
	char	buf[MAX_CMD_LEN];

	if (mathomatic->test_mode || mathomatic->demo_mode) {
		return true;
	}
	mathomatic->show_usage = false;
	if (*cp == '\0') {
		cp = _("Please press the Enter key");
	}
	set_color(mathomatic, 3);	/* make color blue, to show that this is not part of the surrounding text */
	snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), "==== %s: ", cp);
	cp1 = get_string(mathomatic, buf, sizeof(buf));
	default_color(mathomatic, false);
	if (cp1 == NULL) {
		return false;
	}
	if (strncasecmp(cp1, "quit", 4) == 0) {
		return false;
	}
	if (strncasecmp(cp1, "exit", 4) == 0) {
		return false;
	}
	return true;
#endif
}

/*
 * The copy command.
 */
int
copy_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, j, k;
	int	i1;
	char	exists[N_EQUATIONS];
	char	*cp1;
	int	select_flag = false;	/* select the first equation space copied to */

	CLEAR_ARRAY(exists);
	for (i1 = 0; i1 < mathomatic->n_equations; i1++) {
		if (mathomatic->n_lhs[i1] > 0) {
			exists[i1] = true;
		}
	}
	if (strncasecmp(cp, "select", 3) == 0) {
		select_flag = true;
		cp = skip_param(cp);
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &i, &j)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid equation number range."));
			return false;
		}
		for (i1 = i; i1 <= j; i1++) {
			if (exists[i1]) {
				k = next_espace(mathomatic);
				copy_espace(mathomatic, i1, k);
				if (!return_result(mathomatic, k)) {
					return false;
				}
				if (select_flag) {
					mathomatic->cur_equation = k;
					select_flag = false;
				}
			}
		}
	} while (*cp);
	return true;
}

/*
 * Common function for the imaginary and real commands.
 */
static int
complex_func(MathoMatic* mathomatic, char *cp, int imag_flag)
//char	*cp;		/* the command-line */
//int	imag_flag;	/* if true, copy the imaginary part, otherwise copy the real part */
{
	int		i, j, k;
	int		beg;
	int		found_imag, has_imag, has_real, solved;
	token_type	*source, *dest;
	int		n1, *nps, *np;
	long		v = IMAGINARY;			/* separation variable */

	if (current_not_defined(mathomatic)) {
		return false;
	}
	solved = solved_equation(mathomatic, mathomatic->cur_equation);
	i = mathomatic->cur_equation;
	j = next_espace(mathomatic);
	if (mathomatic->n_rhs[i]) {
		source = mathomatic->rhs[i];
		nps = &mathomatic->n_rhs[i];
		dest = mathomatic->rhs[j];
		np = &mathomatic->n_rhs[j];
	} else {
		source = mathomatic->lhs[i];
		nps = &mathomatic->n_lhs[i];
		dest = mathomatic->lhs[j];
		np = &mathomatic->n_lhs[j];
	}
	if (*cp) {
		cp = parse_var2(mathomatic, &v, cp);
		if (cp == NULL) {
			return false;
		}
		if (extra_characters(mathomatic, cp))
			return false;
	}
	simp_loop(mathomatic, source, nps);
	uf_simp(mathomatic, source, nps);
	factorv(mathomatic, source, nps, v);
	mathomatic->partial_flag = false;
	uf_simp(mathomatic, source, nps);
	mathomatic->partial_flag = true;
	n1 = 1;
	dest[0] = mathomatic->zero_token;
	has_imag = has_real = false;
	for (beg = k = 0; beg < *nps; beg = k, k++) {
		for (found_imag = false; k < *nps; k++) {
			if (source[k].level == 1 && source[k].kind == OPERATOR
			    && (source[k].token.operatr == PLUS || source[k].token.operatr == MINUS)) {
				break;
			}
			if (source[k].kind == VARIABLE && source[k].token.variable == v) {
				found_imag = true;
			}
		}
		if (found_imag)
			has_imag = true;
		else
			has_real = true;
		if (found_imag == imag_flag) {
			if (beg == 0) {
				n1 = 0;
			}
			blt(&dest[n1], &source[beg], (k - beg) * sizeof(token_type));
			n1 += (k - beg);
		}
	}
	if (!has_imag || !has_real) {
		warning(mathomatic, _("Expression was not a mix."));
	}
	do {
		simp_loop(mathomatic, dest, &n1);
	} while (factor_plus(mathomatic, dest, &n1, v, 0.0));
	simp_divide(mathomatic, dest, &n1);
	if (mathomatic->n_rhs[i]) {
		blt(mathomatic->lhs[j], mathomatic->lhs[i], mathomatic->n_lhs[i] * sizeof(token_type));
		mathomatic->n_lhs[j] = mathomatic->n_lhs[i];
		if (solved) {
			if (list_var(mathomatic, mathomatic->lhs[j][0].token.variable, 0) < (MAX_VAR_LEN - 5)) {
				if (imag_flag)
					strcat(mathomatic->var_str, "_imag");
				else
					strcat(mathomatic->var_str, "_real");
				parse_var(mathomatic, &mathomatic->lhs[j][0].token.variable, mathomatic->var_str);
			}
		}
	}
	*np = n1;
	mathomatic->cur_equation = j;
	return return_result(mathomatic, mathomatic->cur_equation);
}

/*
 * The real command.
 */
int
real_cmd(MathoMatic* mathomatic, char *cp)
{
	return complex_func(mathomatic, cp, false);
}

/*
 * The imaginary command.
 */
int
imaginary_cmd(MathoMatic* mathomatic, char *cp)
{
	return complex_func(mathomatic, cp, true);
}

#if	!LIBRARY
/*
 * The tally command.
 */
int
tally_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, k, first, last;
	double	count = 0.0;
	int	arithmetic_mean = false;
	long	v;
	char	*cp1;

	if (!parse_var(mathomatic, &v, "total")) {
		return false;
	}
	if (strcmp_tospace(cp, "average") == 0) {
		arithmetic_mean = true;
		cp = skip_param(cp);
	}
	mathomatic->trhs[0] = mathomatic->zero_token;
	mathomatic->n_trhs = 1;
	if (*cp) {
		do {
			cp1 = cp;
			if (!get_range(mathomatic, &cp, &first, &last)) {
				return false;
			}
			if (*cp && cp == cp1) {
				error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
				return false;
			}
			for (k = first; k <= last; k++) {
				if (mathomatic->n_lhs[k] <= 0)
					continue;
				if (mathomatic->n_rhs[k] > 0) {
					if ((mathomatic->n_trhs + 1 + mathomatic->n_rhs[k]) > mathomatic->n_tokens) {
						error_huge(mathomatic);
					}
					for (i = 0; i < mathomatic->n_trhs; i++) {
						mathomatic->trhs[i].level++;
					}
					mathomatic->trhs[mathomatic->n_trhs].kind = OPERATOR;
					mathomatic->trhs[mathomatic->n_trhs].level = 1;
					mathomatic->trhs[mathomatic->n_trhs].token.operatr = PLUS;
					mathomatic->n_trhs++;
					blt(&mathomatic->trhs[mathomatic->n_trhs], mathomatic->rhs[k], mathomatic->n_rhs[k] * sizeof(token_type));
					mathomatic->n_trhs += mathomatic->n_rhs[k];
				} else {
					if ((mathomatic->n_trhs + 1 + mathomatic->n_lhs[k]) > mathomatic->n_tokens) {
						error_huge(mathomatic);
					}
					for (i = 0; i < mathomatic->n_trhs; i++) {
						mathomatic->trhs[i].level++;
					}
					mathomatic->trhs[mathomatic->n_trhs].kind = OPERATOR;
					mathomatic->trhs[mathomatic->n_trhs].level = 1;
					mathomatic->trhs[mathomatic->n_trhs].token.operatr = PLUS;
					mathomatic->n_trhs++;
					blt(&mathomatic->trhs[mathomatic->n_trhs], mathomatic->lhs[k], mathomatic->n_lhs[k] * sizeof(token_type));
					mathomatic->n_trhs += mathomatic->n_lhs[k];
				}
				for (i++; i < mathomatic->n_trhs; i++) {
					mathomatic->trhs[i].level++;
				}
				calc_simp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
				count++;
			}
		} while (*cp);
	}
	if (extra_characters(mathomatic, cp)) {
		return false;
	}
	for (;; count++) {
		fprintf(mathomatic->gfp, _("total = "));
		list_proc(mathomatic, mathomatic->trhs, mathomatic->n_trhs, false);
		fprintf(mathomatic->gfp, "\n");
		if (count > 0) {
			if (arithmetic_mean) {
				/* calculate and display the average */
				blt(mathomatic->tlhs, mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
				mathomatic->n_tlhs = mathomatic->n_trhs;
				if ((mathomatic->n_tlhs + 2) > mathomatic->n_tokens) {
					error_huge(mathomatic);
				}
				for (i = 0; i < mathomatic->n_tlhs; i++) {
					mathomatic->tlhs[i].level++;
				}
				mathomatic->tlhs[mathomatic->n_tlhs].kind = OPERATOR;
				mathomatic->tlhs[mathomatic->n_tlhs].level = 1;
				mathomatic->tlhs[mathomatic->n_tlhs].token.operatr = DIVIDE;
				mathomatic->n_tlhs++;
				mathomatic->tlhs[mathomatic->n_tlhs].kind = CONSTANT;
				mathomatic->tlhs[mathomatic->n_tlhs].level = 1;
				mathomatic->tlhs[mathomatic->n_tlhs].token.constant = count;
				mathomatic->n_tlhs++;
				calc_simp(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
				fprintf(mathomatic->gfp, _("count = %.0f, average = "), count);
				list_proc(mathomatic, mathomatic->tlhs, mathomatic->n_tlhs, false);
				fprintf(mathomatic->gfp, "\n");
			}
		}
		fprintf(mathomatic->gfp, "\n");
		my_strlcpy(mathomatic->prompt_str, _("Enter value to add: "), sizeof(mathomatic->prompt_str));
		if (!get_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs)) {
			break;
		}
		if ((mathomatic->n_trhs + 1 + mathomatic->n_tlhs) > mathomatic->n_tokens) {
			error_huge(mathomatic);
		}
		for (i = 0; i < mathomatic->n_tlhs; i++) {
			mathomatic->tlhs[i].level++;
		}
		for (i = 0; i < mathomatic->n_trhs; i++) {
			mathomatic->trhs[i].level++;
		}
		mathomatic->trhs[mathomatic->n_trhs].kind = OPERATOR;
		mathomatic->trhs[mathomatic->n_trhs].level = 1;
		mathomatic->trhs[mathomatic->n_trhs].token.operatr = PLUS;
		mathomatic->n_trhs++;
		blt(&mathomatic->trhs[mathomatic->n_trhs], mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
		mathomatic->n_trhs += mathomatic->n_tlhs;
		calc_simp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
	}
	fprintf(mathomatic->gfp, _("End.\n"));
	if (count > 0) {
		i = next_espace(mathomatic);
		mathomatic->lhs[i][0].level = 1;
		mathomatic->lhs[i][0].kind = VARIABLE;
		mathomatic->lhs[i][0].token.variable = v;
		mathomatic->n_lhs[i] = 1;
		blt(mathomatic->rhs[i], mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
		mathomatic->n_rhs[i] = mathomatic->n_trhs;
		mathomatic->cur_equation = i;
		return return_result(mathomatic, mathomatic->cur_equation);
	}
	return true;
}
#endif

#if	!LIBRARY
/*
 * The calculate command.
 */
int
calculate_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i, k1, k;
	int		first, last;
	long		v, last_v, it_v = 0;	/* Mathomatic variables */
	long		counter, counter_max;
	sign_array_type	sa_mark, sa_value;
	long		l, iterations = 1;
	token_type	*source;
	int		n;
	int		diff_sign;
	char		buf[MAX_CMD_LEN];
	int		factor_flag = false, value_entered;

	for (;; cp = skip_param(cp)) {
		if (strcmp_tospace(cp, "factor") == 0) {
			factor_flag = true;
			continue;
		}
		break;
	}
	if (!get_range(mathomatic, &cp, &first, &last)) {
		return false;
	} 
	if (*cp) {
		cp = parse_var2(mathomatic, &it_v, cp);
		if (cp == NULL) {
			return false;
		}
		if (*cp == '\0') {
			my_strlcpy(mathomatic->prompt_str, _("Enter maximum number of iterations: "), sizeof(mathomatic->prompt_str));
			if ((cp = get_string(mathomatic, buf, sizeof(buf))) == NULL)
				return false;
		}
		iterations = decstrtol(cp, &cp);
		if (*cp || iterations < 0) {
			error(mathomatic, _("Positive integer required."));
			return false;
		}
		if (iterations == 0) {
			warning(mathomatic, _("Feedback calculation will be iterated until convergence."));
			iterations = LONG_MAX - 1L;
		}
	}
	if (extra_characters(mathomatic, cp)) {
		return false;
	}
calc_again:
	value_entered = false;
	for (i = first; i <= last; i++) {
		if (mathomatic->n_rhs[i]) {
			source = mathomatic->rhs[i];
			n = mathomatic->n_rhs[i];
		} else {
			source = mathomatic->lhs[i];
			n = mathomatic->n_lhs[i];
		}
		if (it_v) {
			if (!found_var(source, n, it_v)) {
				debug_string(mathomatic, (first == last) ? 0 : 1, _("Specified feedback variable not found."));
				continue;
			}
		}
		mathomatic->n_trhs = n;
		blt(mathomatic->trhs, source, mathomatic->n_trhs * sizeof(token_type));
		last_v = 0;
		for (;;) {
			v = -1;
			for (k1 = 0; k1 < n; k1 += 2) {
				if (source[k1].kind == VARIABLE) {
					if (source[k1].token.variable > last_v
					    && (v == -1 || source[k1].token.variable < v))
						v = source[k1].token.variable;
				}
			}
			if (v == -1)
				break;
			last_v = v;
			if ((v & VAR_MASK) <= SIGN || v == it_v) {
				continue;
			}
			if (mathomatic->test_mode || mathomatic->demo_mode)
				continue;
			list_var(mathomatic, v, 0);
			snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("Enter %s: "), mathomatic->var_str);
			if (!get_expr(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs)) {
				continue;
			}
			value_entered = true;
			/* Disguise all variables in the entered expression by making them negative. */
			/* That way they won't be improperly substituted in the future. */
			for (k1 = 0; k1 < mathomatic->n_tlhs; k1 += 2)
				if (mathomatic->tlhs[k1].kind == VARIABLE)
					mathomatic->tlhs[k1].token.variable = -mathomatic->tlhs[k1].token.variable;
			subst_var_with_exp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, mathomatic->tlhs, mathomatic->n_tlhs, v);
		}
		/* Restore disguised variables: */
		for (k1 = 0; k1 < mathomatic->n_trhs; k1 += 2)
			if (mathomatic->trhs[k1].kind == VARIABLE && mathomatic->trhs[k1].token.variable < 0)
				mathomatic->trhs[k1].token.variable = -mathomatic->trhs[k1].token.variable;
		if (it_v) {
			/* Handle the iteration option, where the simplified result is repeatedly plugged into variable it_v. */
			list_var(mathomatic, it_v, 0);
			snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("Enter initial %s: "), mathomatic->var_str);
			while (!get_expr(mathomatic, mathomatic->tes, &mathomatic->n_tes)) {
				printf("Aborted.\n");
				return mathomatic->repeat_flag;
			}
			value_entered = true;
			calc_simp(mathomatic, mathomatic->tes, &mathomatic->n_tes);
			blt(mathomatic->tlhs, mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
			mathomatic->n_tlhs = mathomatic->n_trhs;
			for (l = 0;; l++) {
				if (l >= iterations) {
					fprintf(mathomatic->gfp, _("%ld feedback iterations performed.\n"), l);
					break;
				}
				side_debug(mathomatic, 1, mathomatic->tes, mathomatic->n_tes);
				blt(mathomatic->trhs, mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
				mathomatic->n_trhs = mathomatic->n_tlhs;
				subst_var_with_exp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, mathomatic->tes, mathomatic->n_tes, it_v);
				calc_simp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
				if (se_compare(mathomatic, mathomatic->trhs, mathomatic->n_trhs, mathomatic->tes, mathomatic->n_tes, &diff_sign) && !diff_sign) {
					fprintf(mathomatic->gfp, _("Convergence reached after %ld iterations.\n"), l + 1);
					break;
				}
				blt(mathomatic->tes, mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
				mathomatic->n_tes = mathomatic->n_trhs;
			}
		}
		calc_simp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);

		/* Now substitute all sign variables with +1 and -1. */
		CLEAR_ARRAY(sa_mark);
		for (k1 = 0; k1 < mathomatic->n_trhs; k1 += 2) {
			if (mathomatic->trhs[k1].kind == VARIABLE && (mathomatic->trhs[k1].token.variable & VAR_MASK) == SIGN) {
				sa_mark[(mathomatic->trhs[k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
			}
		}
		for (k1 = 0, k = 0; k1 < ARR_CNT(sa_mark); k1++) {
			if (sa_mark[k1]) {
				k++;
			}
		}
		counter_max = (1L << k) - 1L;
		if (counter_max) {
			fprintf(mathomatic->gfp, _("There are %ld solutions.\n"), counter_max + 1);
		}
		for (counter = 0; counter <= counter_max; counter++) {
			blt(mathomatic->tlhs, mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
			mathomatic->n_tlhs = mathomatic->n_trhs;
			for (k1 = 0, k = 0; k1 < ARR_CNT(sa_mark); k1++) {
				if (sa_mark[k1]) {
					sa_value[k1] = (((1L << k) & counter) != 0);
					k++;
				}
			}
			for (k1 = 0; k1 < mathomatic->n_tlhs; k1 += 2) {
				if (mathomatic->tlhs[k1].kind == VARIABLE && (mathomatic->tlhs[k1].token.variable & VAR_MASK) == SIGN) {
					if (sa_value[(mathomatic->tlhs[k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK]) {
						mathomatic->tlhs[k1].kind = CONSTANT;
						mathomatic->tlhs[k1].token.constant = -1.0;
					} else {
						mathomatic->tlhs[k1].kind = CONSTANT;
						mathomatic->tlhs[k1].token.constant = 1.0;
					}
				}
			}
			for (k1 = 0, k = false; k1 < ARR_CNT(sa_mark); k1++) {
				if (sa_mark[k1]) {
					if (k) {
						fprintf(mathomatic->gfp, ", ");
					} else {
						fprintf(mathomatic->gfp, _("\nSolution number %ld with "), counter + 1);
					}
					list_var(mathomatic, (long) SIGN + (((long) k1) << VAR_SHIFT), 0);
					fprintf(mathomatic->gfp, "%s = ", mathomatic->var_str);
					if (sa_value[k1]) {
						fprintf(mathomatic->gfp, "-1");
					} else {
						fprintf(mathomatic->gfp, "1");
					}
					k = true;
				}
			}
			if (k)
				fprintf(mathomatic->gfp, ":\n");
			calc_simp(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
			if (factor_flag) {
				mid_simp_side(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
			}
			fprintf(mathomatic->gfp, " ");
			if (mathomatic->n_rhs[i]) {
				list_proc(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i], false);
				fprintf(mathomatic->gfp, " = ");
			}
			list_factor(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, factor_flag);
			if (mathomatic->fractions_display && mathomatic->n_tlhs <= 9 && make_fractions(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs)) {
				group_proc(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
				fprintf(mathomatic->gfp, ", with fractions it is: ");
				list_factor(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, factor_flag);
			}
			fprintf(mathomatic->gfp, "\n");
		}
	}
	if (value_entered && mathomatic->repeat_flag) {
		fprintf(mathomatic->gfp, "Repeating:\n");
		goto calc_again;
	}
	return true;
}
#endif

/*
 * The clear command.
 */
int
clear_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, j;
	char	*cp1;

	do {
		cp1 = cp;
		if (is_all(cp)) {
			clear_all(mathomatic);
			return true;
		} else {
			if (!get_range(mathomatic, &cp, &i, &j)) {
				return false;
			}
			if (*cp && cp == cp1) {
				error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
				return false;
			}
			for (; i <= j; i++) {
				mathomatic->n_lhs[i] = 0;
				mathomatic->n_rhs[i] = 0;
			}
		}
	} while (*cp);
	return true;
}

/*
 * Compare the Right Hand Sides of two equation spaces.
 */
static int
compare_rhs(MathoMatic* mathomatic, int i, int j, int *diff_signp)
{
	int	rv;

/* First, test the compare function by comparing with self: */
	rv = se_compare(mathomatic, mathomatic->rhs[i], mathomatic->n_rhs[i], mathomatic->rhs[i], mathomatic->n_rhs[i], diff_signp);
	if (!rv || *diff_signp) {
		error(mathomatic, _("Too many terms to compare."));
		return false;
	}
/* Now do the requested compare: */
	mathomatic->sign_cmp_flag = true;
	rv = se_compare(mathomatic, mathomatic->rhs[i], mathomatic->n_rhs[i], mathomatic->rhs[j], mathomatic->n_rhs[j], diff_signp);
	mathomatic->sign_cmp_flag = false;
	return rv;
}

/*
 * Compare two equation spaces.
 *
 * Return 0 if they differ.
 * Return 1 if identical.  Return -1 if they are expressions that differ only in sign.
 */
int
compare_es(MathoMatic* mathomatic, int i, int j)
//int	i, j;	/* equation space numbers */
{
	int	rv;
	int	diff_sign_lhs, diff_sign_rhs;

	if (mathomatic->n_lhs[i] == 0 || mathomatic->n_lhs[j] == 0)
		return false;	/* empty equation space */
	if ((mathomatic->n_rhs[i] == 0) != (mathomatic->n_rhs[j] == 0))
		return false;	/* mix of expression and equation */
/* Compare the two left hand sides: */
	mathomatic->sign_cmp_flag = true;
	rv = se_compare(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i], mathomatic->lhs[j], mathomatic->n_lhs[j], &diff_sign_lhs);
	mathomatic->sign_cmp_flag = false;
	if (!rv)
		return false;
	if (mathomatic->n_rhs[i] == 0 && mathomatic->n_rhs[j] == 0) {
		/* two expressions, not equations */
		if (diff_sign_lhs)
			return -1;
		else
			return 1;
	}
/* They are equations, so compare the two right hand sides: */
	mathomatic->sign_cmp_flag = true;
	rv = se_compare(mathomatic, mathomatic->rhs[i], mathomatic->n_rhs[i], mathomatic->rhs[j], mathomatic->n_rhs[j], &diff_sign_rhs);
	mathomatic->sign_cmp_flag = false;
	if (!rv)
		return false;
	return(diff_sign_lhs == diff_sign_rhs);
}

/*
 * The compare command.
 */
int
compare_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i, j;
	int		diff_sign;
	int		symb = false, approx = false;

	for (;; cp = skip_param(cp)) {
		if (strncasecmp(cp, "symbolic", 4) == 0) {
			symb = true;
			continue;
		}
		if (strncasecmp(cp, "approximate", 4) == 0) {
			approx = true;
			continue;
		}
		break;
	}
	if (strcmp_tospace(cp, "with") == 0) {
		cp = skip_param(cp);
	}
	i = decstrtol(cp, &cp) - 1;
	if (not_defined(mathomatic, i)) {
		return false;
	}
	if (strcmp_tospace(cp, "with") == 0) {
		cp = skip_param(cp);
	}
	if ((j = get_default_en(mathomatic, cp)) < 0) {
		return false;
	}
	if (i == j) {
		error(mathomatic, _("Cannot compare an expression with itself."));
		return false;
	}
	mathomatic->show_usage = false;
	fprintf(mathomatic->gfp, _("Comparing #%d with #%d...\n"), i + 1, j + 1);
	simp_equation(mathomatic, i);
	simp_equation(mathomatic, j);
	if (mathomatic->n_rhs[i] == 0 || mathomatic->n_rhs[j] == 0) {
		if (mathomatic->n_rhs[i] == 0 && mathomatic->n_rhs[j] == 0) {
			switch (compare_es(mathomatic, i, j)) {
			case 1:
				fprintf(mathomatic->gfp, _("Expressions are identical.\n"));
				return true;
			case -1:
				error(mathomatic, _("Expressions differ only in sign (times -1)."));
				return false;
			}
			if (approx) {
				debug_string(mathomatic, 0, _("Approximating both expressions..."));
				approximate(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
				approximate(mathomatic, mathomatic->lhs[j], &mathomatic->n_lhs[j]);
				switch (compare_es(mathomatic, i, j)) {
				case 1:
					fprintf(mathomatic->gfp, _("Expressions are identical.\n"));
					return true;
				case -1:
					error(mathomatic, _("Expressions differ only in sign (times -1)."));
					return false;
				}
			}
			debug_string(mathomatic, 0, _("Simplifying both expressions..."));
			mathomatic->symb_flag = symb;
			simpa_repeat_side(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i], false, true);
			simpa_repeat_side(mathomatic, mathomatic->lhs[j], &mathomatic->n_lhs[j], false, true);
			mathomatic->symb_flag = false;
			if (approx) {
				approximate(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
				approximate(mathomatic, mathomatic->lhs[j], &mathomatic->n_lhs[j]);
			}
			switch (compare_es(mathomatic, i, j)) {
			case 1:
				fprintf(mathomatic->gfp, _("Expressions are identical.\n"));
				return true;
			case -1:
				error(mathomatic, _("Expressions differ only in sign (times -1)."));
				return false;
			}
#if	!SILENT
			if (mathomatic->debug_level >= 0) {
				list_sub(mathomatic, i);
				list_sub(mathomatic, j);
			}
#endif
			uf_simp(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
			uf_simp(mathomatic, mathomatic->lhs[j], &mathomatic->n_lhs[j]);
			if (approx) {
				approximate(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
				approximate(mathomatic, mathomatic->lhs[j], &mathomatic->n_lhs[j]);
			}
			switch (compare_es(mathomatic, i, j)) {
			case 1:
				fprintf(mathomatic->gfp, _("Expressions are identical.\n"));
				return true;
			case -1:
				error(mathomatic, _("Expressions differ only in sign (times -1)."));
				return false;
			}
			fprintf(mathomatic->gfp, _("Expressions differ.\n"));
			return false;
		}
		error(mathomatic, _("Cannot compare an equation with a non-equation."));
		return false;
	}
	if (compare_es(mathomatic, i, j) > 0) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	if (solved_equation(mathomatic, i) && solved_equation(mathomatic, j)) {
		if (compare_rhs(mathomatic, i, j, &diff_sign)) {
			goto times_neg1;
		}
		if (approx) {
			debug_string(mathomatic, 0, _("Approximating both equations..."));
			approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
			approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
			if (compare_rhs(mathomatic, i, j, &diff_sign)) {
				goto times_neg1;
			}
		}
		debug_string(mathomatic, 0, _("Simplifying both equations..."));
		mathomatic->symb_flag = symb;
		simpa_repeat_side(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], false, true);
		simpa_repeat_side(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j], false, true);
		mathomatic->symb_flag = false;
		if (approx) {
			approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
			approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
		}
		if (compare_rhs(mathomatic, i, j, &diff_sign)) {
			goto times_neg1;
		}
#if	!SILENT
		if (mathomatic->debug_level >= 0) {
			list_sub(mathomatic, i);
			list_sub(mathomatic, j);
		}
#endif
		uf_simp(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
		uf_simp(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
		if (approx) {
			approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
			approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
		}
		if (compare_rhs(mathomatic, i, j, &diff_sign)) {
			goto times_neg1;
		}
	}
	debug_string(mathomatic, 0, _("Solving both equations for zero and expanding..."));
	if (solve_sub(mathomatic, &mathomatic->zero_token, 1, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->rhs[i], &mathomatic->n_rhs[i]) <= 0)
		return false;
	if (solve_sub(mathomatic, &mathomatic->zero_token, 1, mathomatic->lhs[j], &mathomatic->n_lhs[j], mathomatic->rhs[j], &mathomatic->n_rhs[j]) <= 0)
		return false;
	if (compare_rhs(mathomatic, i, j, &diff_sign)) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	uf_simp(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
	uf_simp(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
	if (compare_rhs(mathomatic, i, j, &diff_sign)) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	if (approx) {
		debug_string(mathomatic, 0, _("Approximating both equations..."));
		approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
		approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
		if (compare_rhs(mathomatic, i, j, &diff_sign)) {
			fprintf(mathomatic->gfp, _("Equations are identical.\n"));
			return true;
		}
	}
	debug_string(mathomatic, 0, _("Simplifying both equations..."));
	mathomatic->symb_flag = symb;
	simpa_repeat_side(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i], false, false);
	simpa_repeat_side(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j], false, false);
	mathomatic->symb_flag = false;
	if (approx) {
		approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
		approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
	}
	if (compare_rhs(mathomatic, i, j, &diff_sign)) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	if (solve_sub(mathomatic, &mathomatic->zero_token, 1, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->rhs[i], &mathomatic->n_rhs[i]) <= 0)
		return false;
	if (solve_sub(mathomatic, &mathomatic->zero_token, 1, mathomatic->lhs[j], &mathomatic->n_lhs[j], mathomatic->rhs[j], &mathomatic->n_rhs[j]) <= 0)
		return false;
	uf_simp(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
	uf_simp(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
	if (approx) {
		approximate(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
		approximate(mathomatic, mathomatic->rhs[j], &mathomatic->n_rhs[j]);
	}
	if (compare_rhs(mathomatic, i, j, &diff_sign)) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	fprintf(mathomatic->gfp, _("Equations differ.\n"));
	return false;
times_neg1:
	if (!diff_sign && mathomatic->lhs[i][0].token.variable == mathomatic->lhs[j][0].token.variable) {
		fprintf(mathomatic->gfp, _("Equations are identical.\n"));
		return true;
	}
	fprintf(mathomatic->gfp, _("Variable "));
	list_proc(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i], false);
	fprintf(mathomatic->gfp, _(" in the first equation\nis equal to "));
	if (diff_sign) {
		fprintf(mathomatic->gfp, "-");
	}
	list_proc(mathomatic, mathomatic->lhs[j], mathomatic->n_lhs[j], false);
	fprintf(mathomatic->gfp, _(" in the second equation.\n"));
#if	LIBRARY
	if (diff_sign)
		error(mathomatic, _("RHS appears negated."));
	else
		error(mathomatic, _("Different LHS variable name, otherwise the same."));
	return false;
#else
	return 2;
#endif
}

/*
 * Display the specified floating point value.
 * If it is equal to a simple fraction, display that too.
 *
 * Return true if a fraction was displayed.
 */
int
display_fraction(MathoMatic* mathomatic, double value)
{
	double	d4, d5;
	int	rv = false;

	f_to_fraction(mathomatic, value, &d4, &d5);
	fprintf(mathomatic->gfp, "%.*g", mathomatic->precision, value);
	if (d5 != 1.0) {
		fprintf(mathomatic->gfp, " = %.*g/%.*g", mathomatic->precision, d4, mathomatic->precision, d5);
		rv = true;
	}
	fprintf(mathomatic->gfp, "\n");
	return rv;
}

/*
 * The divide command.
 */
int
divide_cmd(MathoMatic* mathomatic, char *cp)
{
	long		v = 0, v_tmp;		/* Mathomatic variables */
	int		i, j;
	int		nleft = 0, nright = 0;
	double		lcm, d1, d2, d3, d4, d5;
	complexs	c1, c2, c3;
	char		*cp_start;

	cp_start = cp;
	mathomatic->pull_number = -1;	/* Operands are last two entered expressions when using library. */
	if (*cp && isvarchar(mathomatic, *cp)) {
		cp = parse_var(mathomatic, &v, cp);
		if (cp == NULL || (*cp && !isspace(*cp) && *cp != ',')) {
			if (cp == NULL) {
				reset_error(mathomatic);
			}
			cp = cp_start;
			v = 0;
		} else {
			cp = skip_comma_space(cp);
			SP(mathomatic, ("You have entered a base variable."));
			EP(mathomatic, ("Polynomial division will be based on this variable."));
			mathomatic->point_flag = false;
		}
	}
	i = next_espace(mathomatic);
	if (*cp) {
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->rhs[i], &nright, cp, false);
		if (cp == NULL || nright <= 0) {
			return false;
		}
	}
	if (*cp) {
		cp_start = cp;
		cp = skip_comma_space(cp);
		mathomatic->input_column += (cp - cp_start);
		cp = parse_expr(mathomatic, mathomatic->lhs[i], &nleft, cp, false);
		if (cp == NULL || extra_characters(mathomatic, cp) || nleft <= 0) {
			return false;
		}
	}
do_repeat_prompt:
/* prompt for the two operands */
	my_strlcpy(mathomatic->prompt_str, _("Enter dividend: "), sizeof(mathomatic->prompt_str));
	if (nright == 0 && !get_expr(mathomatic, mathomatic->rhs[i], &nright)) {
		return mathomatic->repeat_flag;
	}
	my_strlcpy(mathomatic->prompt_str, _("Enter divisor: "), sizeof(mathomatic->prompt_str));
	if (nleft == 0 && !get_expr(mathomatic, mathomatic->lhs[i], &nleft)) {
		return mathomatic->repeat_flag;
	}
	fprintf(mathomatic->gfp, "\n");
/* simplify and expand the operand expressions */
#if	1
	simp_loop(mathomatic, mathomatic->rhs[i], &nright);
	uf_simp(mathomatic, mathomatic->rhs[i], &nright);
	simp_loop(mathomatic, mathomatic->lhs[i], &nleft);
	uf_simp(mathomatic, mathomatic->lhs[i], &nleft);
#else
/* approximates, too */
	calc_simp(mathomatic, mathomatic->rhs[i], &nright);
	calc_simp(mathomatic, mathomatic->lhs[i], &nleft);
#endif
/* if division by zero, display a warning */
	if (get_constant(mathomatic, mathomatic->lhs[i], nleft, &d2)) {
		check_divide_by_zero(mathomatic, d2);
	}
/* Do constant division if 2 normal numbers were entered */
	if (get_constant(mathomatic, mathomatic->rhs[i], nright, &d1) && get_constant(mathomatic, mathomatic->lhs[i], nleft, &d2)) {
		fprintf(mathomatic->gfp, _("Result of numerical division:\n"));
		d3 = gcd_verified(mathomatic, d1, d2);
		d4 = modf(d1 / d2, &d5);
		fprintf(mathomatic->gfp, "%.*g/%.*g = %.*g", mathomatic->precision, d1, mathomatic->precision, d2, mathomatic->precision, d1 / d2);
		if (d3 != 0.0 && d3 != 1.0 && (d2 / d3) != 1.0) {
			if ((d1 / d2) < 0) {
				fprintf(mathomatic->gfp, " = -%.*g/%.*g", mathomatic->precision, fabs(d1 / d3), mathomatic->precision, fabs(d2 / d3));
			} else {
				fprintf(mathomatic->gfp, " = %.*g/%.*g", mathomatic->precision, fabs(d1 / d3), mathomatic->precision, fabs(d2 / d3));
			}
		}
		if (d3 != 0 && d4 != 0 && d5 != 0) {
			if ((d1 / d2) < 0) {
				fprintf(mathomatic->gfp, " = -(%.*g + (%.*g/%.*g))", mathomatic->precision, fabs(d5), mathomatic->precision, fabs(d4 * (d2 / d3)), mathomatic->precision, fabs(d2 / d3));
			} else {
				fprintf(mathomatic->gfp, " = %.*g + (%.*g/%.*g)", mathomatic->precision, fabs(d5), mathomatic->precision, fabs(d4 * (d2 / d3)), mathomatic->precision, fabs(d2 / d3));
			}
		}
		fprintf(mathomatic->gfp, _("\nQuotient: %.*g, Remainder: %.*g\n"), mathomatic->precision, d5, mathomatic->precision, d4 * d2);
		d1 = fabs(d1);
		d2 = fabs(d2);
		if (d3 == 0.0) {
			fprintf(mathomatic->gfp, _("No GCD found.\n"));
			if (mathomatic->repeat_flag)
				goto do_repeat;
			return true;
		}
		fprintf(mathomatic->gfp, "GCD = ");
		if (d3 >= 4.0 && factor_one(mathomatic, d3) && !is_prime(mathomatic)) {
			display_unique(mathomatic);
		} else {
			display_fraction(mathomatic, d3);
		}
		lcm = (d1 * d2) / d3;
		fprintf(mathomatic->gfp, "LCM = ");
		if (lcm >= 4.0 && factor_one(mathomatic, lcm) && !is_prime(mathomatic)) {
			display_unique(mathomatic);
		} else {
			display_fraction(mathomatic, lcm);
		}
		if (mathomatic->repeat_flag)
			goto do_repeat;
		return true;
	}
/* else do complex number division if 2 complex numbers were entered */
	if (parse_complex(mathomatic, mathomatic->rhs[i], nright, &c1) && parse_complex(mathomatic, mathomatic->lhs[i], nleft, &c2)) {
		fprintf(mathomatic->gfp, _("Result of complex number division:\n"));
		c3 = complex_div(c1, c2);
		fprintf(mathomatic->gfp, "%.*g %+.*g*i\n\n", mathomatic->precision, c3.re, mathomatic->precision, c3.im);
		if (mathomatic->repeat_flag)
			goto do_repeat;
		return true;
	}
/* else do polynomial division and univariate GCD display */
	v_tmp = v;
	if (poly_div(mathomatic, mathomatic->rhs[i], nright, mathomatic->lhs[i], nleft, &v_tmp)) {
		simp_divide(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
		simp_divide(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
		list_var(mathomatic, v_tmp, 0);
		fprintf(mathomatic->gfp, _("Polynomial division successful using base variable %s.\n"), mathomatic->var_str);
		fprintf(mathomatic->gfp, _("The quotient is:\n"));
		fractions_and_group(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
		list_factor(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, false);
		fprintf(mathomatic->gfp, _("\n\nThe remainder is:\n"));
		fractions_and_group(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
		list_factor(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, false);
		fprintf(mathomatic->gfp, "\n");
	} else {
		SP(mathomatic, ("Polynomial division failed,"));
		SP(mathomatic, ("because the given polynomials cannot be divided in the given order,"));
		EP(mathomatic, ("according to the rules of polynomial division."));
	}
	fprintf(mathomatic->gfp, "\n");
	j = poly_gcd(mathomatic, mathomatic->rhs[i], nright, mathomatic->lhs[i], nleft, v);
	if (j == 0) {
		j = poly_gcd(mathomatic, mathomatic->lhs[i], nleft, mathomatic->rhs[i], nright, v);
	}
	if (j > 0) {
		simp_divide(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
		fprintf(mathomatic->gfp, _("Polynomial GCD (after %d Euclidean algorithm iterations):\n"), j);
		fractions_and_group(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
		list_factor(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, false);
		fprintf(mathomatic->gfp, "\n");
		blt(mathomatic->tes, mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
		mathomatic->n_tes = mathomatic->n_trhs;
		if (poly_factor(mathomatic, mathomatic->tes, &mathomatic->n_tes, true)) {
			simp_loop(mathomatic, mathomatic->tes, &mathomatic->n_tes);
			fprintf(mathomatic->gfp, _("Polynomial GCD (after quick polynomial factoring):\n"));
			fractions_and_group(mathomatic, mathomatic->tes, &mathomatic->n_tes);
			list_factor(mathomatic, mathomatic->tes, &mathomatic->n_tes, false);
			fprintf(mathomatic->gfp, "\n");
		}
	} else {
		SP(mathomatic, ("No additive univariate polynomial GCD found."));
		SP(mathomatic, ("This does not mean there is no GCD; it could be multivariate,"));
		EP(mathomatic, ("or contain too much floating point round-off error."));
	}
	if (mathomatic->repeat_flag)
		goto do_repeat;
	return true;

do_repeat:
	nright = 0;
	nleft = 0;
	goto do_repeat_prompt;
}

/*
 * The eliminate command.
 */
int
eliminate_cmd(MathoMatic* mathomatic, char *cp)
{
	long	v, last_v, v1, va[MAX_VARS];		/* Mathomatic variables */
	int	vc = 0;					/* variable count */
	int	i = 0, n;
	int	success_flag = false, did_something = false, using_flag;
	char	used[N_EQUATIONS];
	char	*cp_start;
	char	buf[MAX_CMD_LEN];

	CLEAR_ARRAY(used);
	if (current_not_defined(mathomatic)) {
		return false;
	}
	if (*cp == '\0') {
		my_strlcpy(mathomatic->prompt_str, _("Enter variables to eliminate: "), sizeof(mathomatic->prompt_str));
		cp = get_string(mathomatic, buf, sizeof(buf));
		if (cp == NULL || *cp == '\0') {
			return false;
		}
	}
	cp_start = cp;
next_var:
	if (vc) {
		v = va[--vc];
	} else if (*cp) {
		if (is_all(cp)) {
			cp = skip_param(cp);
			vc = 0;
			last_v = 0;
			for (;;) {
				v1 = -1;
				for (i = 0; i < mathomatic->n_lhs[mathomatic->cur_equation]; i += 2) {
					if (mathomatic->lhs[mathomatic->cur_equation][i].kind == VARIABLE
					    && mathomatic->lhs[mathomatic->cur_equation][i].token.variable > last_v) {
						if (v1 == -1 || mathomatic->lhs[mathomatic->cur_equation][i].token.variable < v1) {
							v1 = mathomatic->lhs[mathomatic->cur_equation][i].token.variable;
						}
					}
				}
				for (i = 0; i < mathomatic->n_rhs[mathomatic->cur_equation]; i += 2) {
					if (mathomatic->rhs[mathomatic->cur_equation][i].kind == VARIABLE
					    && mathomatic->rhs[mathomatic->cur_equation][i].token.variable > last_v) {
						if (v1 == -1 || mathomatic->rhs[mathomatic->cur_equation][i].token.variable < v1) {
							v1 = mathomatic->rhs[mathomatic->cur_equation][i].token.variable;
						}
					}
				}
				if (v1 == -1)
					break;
				last_v = v1;
				if ((v1 & VAR_MASK) > SIGN) {
					if (vc >= ARR_CNT(va)) {
						break;
					}
					va[vc++] = v1;
				}
			}
			goto next_var;
		}
		cp = parse_var2(mathomatic, &v, cp);
		if (cp == NULL) {
			return false;
		}
	} else {
		if (mathomatic->repeat_flag) {
			if (success_flag) {
				success_flag = false;
				cp = cp_start;
				goto next_var;	/* repeat until failure to substitute anything */
			}
		}
		if (did_something) {
			did_something = return_result(mathomatic, mathomatic->cur_equation);
		} else {
			error(mathomatic, _("No substitutions made."));
		}
		return did_something;
	}
	using_flag = (strcmp_tospace(cp, "using") == 0);
	if (using_flag) {
		cp = skip_param(cp);
		if (*cp == '#')
			cp++;
		i = decstrtol(cp, &cp) - 1;
		if (not_defined(mathomatic, i)) {
			return false;
		}
	}
	if (!var_in_equation(mathomatic, mathomatic->cur_equation, v)) {
#if	!SILENT
		if (!mathomatic->repeat_flag) {
			list_var(mathomatic, v, 0);
			printf(_("Variable %s not found in current equation.\n"), mathomatic->var_str);
		}
#endif
		goto next_var;
	}
	if (using_flag) {
		if (!elim_sub(mathomatic, i, v))
			goto next_var;
	} else {
		n = 1;
		i = mathomatic->cur_equation;
		for (;; n++) {
			if (n >= mathomatic->n_equations) {
				goto next_var;
			}
			if (i <= 0)
				i = mathomatic->n_equations - 1;
			else
				i--;
			if (used[i])
				continue;
			if (mathomatic->n_lhs[i] && mathomatic->n_rhs[i] && var_in_equation(mathomatic, i, v)) {
				if (elim_sub(mathomatic, i, v))
					break;
			}
		}
	}
	success_flag = true;
	did_something = true;
	used[i] = true;
	goto next_var;
}

/*
 * Solve equation number i for v and substitute the RHS
 * into all occurrences of v in the current equation, then simplify.
 */
static int
elim_sub(MathoMatic* mathomatic, int i, long v)
//int	i;	/* equation number */
//long	v;	/* Mathomatic variable */
{
	token_type	want;
	int		solved;

	if (i == mathomatic->cur_equation) {
		error(mathomatic, _("Error: source and destination are the same."));
		return false;
	}
	solved = (solved_equation(mathomatic, i) && mathomatic->lhs[i][0].token.variable == v);
#if	!SILENT
	list_var(mathomatic, v, 0);
	if (solved) {
		fprintf(mathomatic->gfp, _("Eliminating variable %s using solved equation #%d...\n"), mathomatic->var_str, i + 1);
	} else {
		fprintf(mathomatic->gfp, _("Solving equation #%d for %s and substituting into the current equation...\n"), i + 1, mathomatic->var_str);
	}
#endif
	if (!solved) {
		want.level = 1;
		want.kind = VARIABLE;
		want.token.variable = v;
		if (solve_sub(mathomatic, &want, 1, mathomatic->lhs[i], &mathomatic->n_lhs[i], mathomatic->rhs[i], &mathomatic->n_rhs[i]) <= 0) {
			error(mathomatic, _("Solve failed."));
			return false;
		}
	}
	subst_var_with_exp(mathomatic, mathomatic->rhs[mathomatic->cur_equation], &mathomatic->n_rhs[mathomatic->cur_equation], mathomatic->rhs[i], mathomatic->n_rhs[i], v);
	subst_var_with_exp(mathomatic, mathomatic->lhs[mathomatic->cur_equation], &mathomatic->n_lhs[mathomatic->cur_equation], mathomatic->rhs[i], mathomatic->n_rhs[i], v);

	simp_equation(mathomatic, mathomatic->cur_equation);
	return true;
}

/*
 * The display command.
 *
 * Displays equations in multi-line fraction format.
 *
 * Return number of expressions displayed.
 */
int
display_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, j;
	char	*cp1;
	jmp_buf	save_save;
	int	factor_flag = false, displayed = 0;
	int	orig_fractions_display_mode, new_fractions_display_mode;

	new_fractions_display_mode = orig_fractions_display_mode = mathomatic->fractions_display;
	for (;; cp = skip_param(cp)) {
		if (strncasecmp(cp, "factor", 4) == 0) {
			factor_flag = true;
			continue;
		}
		if (strncasecmp(cp, "simple", 4) == 0) {
			new_fractions_display_mode = 1;
			continue;
		}
		if (strncasecmp(cp, "mixed", 3) == 0) {
			new_fractions_display_mode = 2;
			continue;
		}
		break;
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &i, &j)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (; i <= j; i++) {
			if (mathomatic->n_lhs[i] > 0) {
				blt(save_save, mathomatic->jmp_save, sizeof(mathomatic->jmp_save));
				if (setjmp(mathomatic->jmp_save) != 0) {	/* trap errors */
					mathomatic->fractions_display = orig_fractions_display_mode;
					blt(mathomatic->jmp_save, save_save, sizeof(mathomatic->jmp_save));
					printf("Skipping equation number %d.\n", i + 1);
					continue;
				}
				mathomatic->fractions_display = new_fractions_display_mode;
				make_fractions_and_group(mathomatic, i);
				mathomatic->fractions_display = orig_fractions_display_mode;
				if (factor_flag || mathomatic->factor_int_flag) {
					factor_int_equation(mathomatic, i);
				}
				blt(mathomatic->jmp_save, save_save, sizeof(mathomatic->jmp_save));
#if     LIBRARY
				free_result_str(mathomatic);
				mathomatic->result_str = flist_equation_string(mathomatic, i);
				if (mathomatic->result_str == NULL)
					mathomatic->result_str = list_equation(mathomatic, i, false);
				if (mathomatic->result_str)
					mathomatic->result_en = i;
				if (mathomatic->gfp != stdout) {
					if (flist_equation(mathomatic, i) > 0) {
						displayed++;
					}
				}
#else
				if (flist_equation(mathomatic, i) > 0) {
					displayed++;
				}
#endif
			}
		}
	} while (*cp);
#if	LIBRARY
	return(mathomatic->result_str != NULL);
#else
	return(displayed);
#endif
}

/*
 * The list command.
 */
int
list_cmd(MathoMatic* mathomatic, char *cp)
{
	int	k;
	int	first, last;
	char	*cp1;
	int	export_flag = 0;
#if     SHELL_OUT
	char	cl[MAX_CMD_LEN];
	int	primes_flag = false;
	int	ev;	/* exit value */
#endif

	if (strncasecmp(cp, "gnuplot", 3) == 0) {
		export_flag = 3;
		cp = skip_param(cp);
	} else if (strncasecmp(cp, "export", 3) == 0) {
		export_flag = 2;
		cp = skip_param(cp);
	} else if (strncasecmp(cp, "maxima", 3) == 0) {
		export_flag = 1;
		cp = skip_param(cp);
	} else if (strncasecmp(cp, "hexadecimal", 3) == 0) {
		export_flag = 4;
		cp = skip_param(cp);
#if     SHELL_OUT
	} else if (strncasecmp(cp, "primes", 5) == 0) {
		primes_flag = true;
		cp = skip_param(cp);
#endif
	}
#if     SHELL_OUT
	if (primes_flag) {
		if (mathomatic->gfp && mathomatic->gfp_filename && mathomatic->gfp_filename[0]) {
			if (snprintf(cl, sizeof(cl), "matho-primes -u %s >%s%s", cp, mathomatic->gfp_append_flag ? ">" : "", mathomatic->gfp_filename) >= sizeof(cl)) {
				error(mathomatic, _("Command-line too long."));
				return false;
			}
			clean_up(mathomatic);	/* end any redirection */
		} else {
			if (snprintf(cl, sizeof(cl), "matho-primes -u %s", cp) >= sizeof(cl)) {
				error(mathomatic, _("Command-line too long."));
				return false;
			}
		}
		if ((ev = shell_out(mathomatic, cl))) {
			error(mathomatic, _("Abnormal termination of matho-primes."));
			printf(_("Decimal exit value = %d, shell command-line = %s\n"), ev, cl);
			return false;
		}
		return true;
	}
#endif
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &first, &last)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (k = first; k <= last; k++) {
			if (mathomatic->n_lhs[k] <= 0)
				continue;
#if	LIBRARY
			free_result_str(mathomatic);
			mathomatic->result_str = list_equation(mathomatic, k, export_flag);
			if (mathomatic->result_str)
				mathomatic->result_en = k;
			else
				return false;
			if (mathomatic->gfp == stdout) {
				continue;
			}
#endif
			list1_sub(mathomatic, k, export_flag);
		}
	} while (*cp);
	return true;
}

/*
 * The code command.
 */
int
code_cmd(MathoMatic* mathomatic, char *cp)
{
	int			i, j, k;
	int			li, ri;
	enum language_list	language = C;
	int			int_flag = false, displayed = false;
	char			*cp1;

	for (;; cp = skip_param(cp)) {
		if (strcmp_tospace(cp, "c") == 0 || strcmp_tospace(cp, "c++") == 0) {
			language = C;
			continue;
		}
		if (strcmp_tospace(cp, "java") == 0) {
			language = JAVA;
			continue;
		}
		if (strcmp_tospace(cp, "python") == 0) {
			language = PYTHON;
			continue;
		}
		if (strncasecmp(cp, "integer", 3) == 0) {
			int_flag = true;
			continue;
		}
		break;
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &i, &j)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (k = i; k <= j; k++) {
			if (mathomatic->n_lhs[k] <= 0)
				continue;
			if (mathomatic->n_rhs[k] == 0 || mathomatic->n_lhs[k] != 1 || mathomatic->lhs[k][0].kind != VARIABLE) {
				warning(mathomatic, _("Can't make assignment statement because this is not an equation."));
			} else if (!solved_equation(mathomatic, k)) {
				warning(mathomatic, _("Equation is not solved for a normal variable."));
			}
			simp_i(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
			if (int_flag) {
				/* factor_constants() for more accurate integer results. */
				do {
					simp_loop(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
				} while (factor_constants(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k], 6));
				/* Turn the power operator into the multiply operator, if raised to the power of a constant. */
				uf_repeat_always(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
			}
			if (mathomatic->n_rhs[k] > 0) {
				simp_i(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				if (int_flag) {
					/* factor_constants() for more accurate integer results. */
					do {
						simp_loop(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
					} while (factor_constants(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k], 6));
					/* Turn the power operator into the multiply operator, if raised to the power of a constant. */
					uf_repeat_always(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				}
			}
			make_fractions_and_group(mathomatic, k);
			if (int_flag) {
				if ((!(li = int_expr(mathomatic->lhs[k], mathomatic->n_lhs[k])) || !(ri = int_expr(mathomatic->rhs[k], mathomatic->n_rhs[k])))) {
					warning(mathomatic, _("Not an integer expression, but this rounded code may possibly work:"));
				} else if (li < 0 || ri < 0) {
					warning(mathomatic, _("This integer expression contains non-integer divides:"));
				}
			}
#if	LIBRARY
			free_result_str(mathomatic);
			mathomatic->result_str = string_code_equation(mathomatic, k, language, int_flag);
			if (mathomatic->result_str)
				mathomatic->result_en = k;
			else
				return false;
			if (mathomatic->gfp == stdout) {
				displayed = true;
				continue;
			}
#endif
			if (list_code_equation(mathomatic, k, language, int_flag) > 0) {
				displayed = true;
			}
		}
	} while (*cp);
	return displayed;
}
#ifndef VCMP_DEFINED
#define VCMP_DEFINED /*there is a copy elsewhere */
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
#endif /*VCMP_DEFINED*/
/*
 * The variables command.
 */
int
variables_cmd(MathoMatic* mathomatic, char *cp)
{
	int			start, stop;
	int			k;
	int			i1;
	long			v1, last_v;		/* Mathomatic variables */
	int			vc, cnt;		/* variable counts */
	sort_type		va[MAX_VARS];		/* variable array */
	token_type		*p1;
	int			n1;
	enum language_list	lang_code = 0;		/* default to no programming language */
	int			int_flag = false, imag_flag = false, count_flag = false, not_complex = false;
	char			imag_array[N_EQUATIONS];
	char			*range_start, *cp1;
	int			array_element_flag = false;
	int			rv = false;
	int			n_tabs = 0;

	CLEAR_ARRAY(imag_array);
	if (strncasecmp(cp, "counts", 5) == 0) {
		cp = skip_param(cp);
		count_flag = true;
	}
	if (strcmp_tospace(cp, "c") == 0 || strcmp_tospace(cp, "c++") == 0) {
		cp = skip_param(cp);
		lang_code = C;
	} else if (strcmp_tospace(cp, "java") == 0) {
		cp = skip_param(cp);
		lang_code = JAVA;
	} else if (strncasecmp(cp, "integer", 3) == 0) {
		cp = skip_param(cp);
		lang_code = C;
		int_flag = true;
	}
	if (strncasecmp(cp, "counts", 5) == 0) {
		cp = skip_param(cp);
		count_flag = true;
	}
	range_start = cp;
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &start, &stop)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (k = start; k <= stop; k++) {
			if (mathomatic->n_lhs[k] <= 0)
				continue;
			if (mathomatic->n_rhs[k] > 0) {
				p1 = mathomatic->rhs[k];
				n1 = mathomatic->n_rhs[k];
			} else {
				p1 = mathomatic->lhs[k];
				n1 = mathomatic->n_lhs[k];
			}
			for (i1 = 0; i1 < n1; i1 += 2) {
				if (p1[i1].kind == VARIABLE && p1[i1].token.variable == IMAGINARY) {
					imag_flag = true;
					imag_array[k] = true;
					break;
				}
			}
		}
	} while (*cp);
	mathomatic->show_usage = false;
	last_v = 0;
	for (vc = 0;;) {
		if (vc >= ARR_CNT(va)) {
			error(mathomatic, _("Too many variables to list."));
			return false;
		}
		cnt = 0;
		v1 = -1;
		cp = range_start;
		do {
			cp1 = cp;
			if (!get_range(mathomatic, &cp, &start, &stop)) {
				return false;
			}
#if	DEBUG
			if (*cp && cp == cp1) {
				error_bug(mathomatic, "Bug in variables command.");
			}
#endif
			for (k = start; k <= stop; k++) {
				if (mathomatic->n_lhs[k] <= 0)
					continue;
				p1 = mathomatic->lhs[k];
				n1 = mathomatic->n_lhs[k];
				for (i1 = 0; i1 < n1; i1 += 2) {
					if (p1[i1].kind == VARIABLE && p1[i1].token.variable > last_v) {
						if (v1 == -1 || p1[i1].token.variable < v1) {
							v1 = p1[i1].token.variable;
							cnt = 1;
						} else if (p1[i1].token.variable == v1) {
							cnt++;
						}
					}
				}
				p1 = mathomatic->rhs[k];
				n1 = mathomatic->n_rhs[k];
				for (i1 = 0; i1 < n1; i1 += 2) {
					if (p1[i1].kind == VARIABLE && p1[i1].token.variable > last_v) {
						if (v1 == -1 || p1[i1].token.variable < v1) {
							v1 = p1[i1].token.variable;
							cnt = 1;
						} else if (p1[i1].token.variable == v1) {
							cnt++;
						}
					}
				}
			}
		} while (*cp);
		if (v1 == -1)
			break;
		last_v = v1;
		va[vc].v = v1;
		va[vc].count = cnt;
		vc++;
	}
	if (vc <= 0) {
		if (lang_code == 0) {
			error(mathomatic, _("Expression is numeric.  No normal variables found."));
			return false;
		} else {
			return true;
		}
	}
	qsort((char *) va, vc, sizeof(*va), vcmp);
	for (i1 = 0; i1 < vc; i1++) {
		if (lang_code && va[i1].v < SIGN) {
			continue;
		}
		if ((va[i1].v & VAR_MASK) >= SIGN) {
			rv = true;
		}
		n_tabs = list_var(mathomatic, va[i1].v, lang_code ? lang_code : -5);
		if (lang_code) {
			if (strpbrk(mathomatic->var_str, "[]()"))
				array_element_flag = true;
			if (imag_flag) {
				for (k = 0;; k++) {
					if (k >= mathomatic->n_equations) {
						not_complex = true;
						break;
					}
					if (imag_array[k] && mathomatic->n_lhs[k] == 1
					    && mathomatic->lhs[k][0].kind == VARIABLE && mathomatic->lhs[k][0].token.variable == va[i1].v) {
						fprintf(mathomatic->gfp, "_Complex ");
						n_tabs += 8;
						break;
					}
				}
			}
			if (int_flag || is_integer_var(mathomatic, va[i1].v) || (va[i1].v & VAR_MASK) == SIGN) {
				fprintf(mathomatic->gfp, "int%s%s;", (n_tabs + 1)/8 ? "\t" : "\t\t", mathomatic->var_str);
			} else {
				fprintf(mathomatic->gfp, "double%s%s;", (n_tabs + 1)/8 ? "\t" : "\t\t", mathomatic->var_str);
			}
			if (n_tabs >= 7)
				n_tabs -= 7;
		} else {
			fprintf(mathomatic->gfp, "%s", mathomatic->var_str);
		}
		if (count_flag) {
			if ((n_tabs / 8) == 0) {
				fprintf(mathomatic->gfp, "\t");
			}
			fprintf(mathomatic->gfp, _("\t/* count = %d */\n"), va[i1].count);
		} else {
			fprintf(mathomatic->gfp, "\n");
		}
	}
	if (lang_code && imag_flag && not_complex && rv) {
		printf("\n");
		warning(mathomatic, _("Some variables might need to be of the complex number type."));
		printf(_("Manual adjustments may be necessary\n"));
		printf(_("because of the appearance of the imaginary unit (i).\n"));
	}
	if (!rv) {
		error(mathomatic, _("Expressions are all numeric.  No variables found."));
	}
	if (array_element_flag) {
		warning(mathomatic, _("Some defined variables were array elements or functions, requiring manual definition."));
		rv = false;
	}
	return rv;
}

/*
 * The approximate command.
 */
int
approximate_cmd(MathoMatic* mathomatic, char *cp)
{
	int	start, stop;
	int	k;
	char	*cp1;

	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &start, &stop)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (k = start; k <= stop; k++) {
			if (mathomatic->n_lhs[k]) {
				approximate(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
				if (mathomatic->n_rhs[k]) {
					approximate(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				}
				if (!return_result(mathomatic, k)) {
					return false;
				}
			}
		}
	} while (*cp);
	return true;
}

/*
 * The replace command.
 */
int
replace_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, j;
	long	last_v, v, va[MAX_VARS];	/* Mathomatic variables */
	int	vc;				/* variable count */
	char	*cp_start, *cp1;
	int	found, value_entered;
	int	diff_sign;

	cp_start = cp;
	if (current_not_defined(mathomatic)) {
		return false;
	}
	i = mathomatic->cur_equation;
	for (vc = 0; *cp; vc++) {
		if (strcmp_tospace(cp, "with") == 0) {
			if (vc) {
				mathomatic->repeat_flag = false;
				break;
			}
		}
		if (vc >= ARR_CNT(va)) {
			error(mathomatic, _("Too many variables specified."));
			return false;
		}
		cp = parse_var2(mathomatic, &va[vc], cp);
		if (cp == NULL) {
			return false;
		}
		if (!var_in_equation(mathomatic, i, va[vc])) {
			error(mathomatic, _("Variable not found."));
			return false;
		}
	}
replace_again:
	mathomatic->n_tlhs = mathomatic->n_lhs[i];
	blt(mathomatic->tlhs, mathomatic->lhs[i], mathomatic->n_tlhs * sizeof(token_type));
	mathomatic->n_trhs = mathomatic->n_rhs[i];
	blt(mathomatic->trhs, mathomatic->rhs[i], mathomatic->n_trhs * sizeof(token_type));
	value_entered = false;
	last_v = 0;
	for (;;) {
		v = -1;
		for (j = 0; j < mathomatic->n_lhs[i]; j += 2) {
			if (mathomatic->lhs[i][j].kind == VARIABLE) {
				if (mathomatic->lhs[i][j].token.variable > last_v
				    && (v == -1 || mathomatic->lhs[i][j].token.variable < v))
					v = mathomatic->lhs[i][j].token.variable;
			}
		}
		for (j = 0; j < mathomatic->n_rhs[i]; j += 2) {
			if (mathomatic->rhs[i][j].kind == VARIABLE) {
				if (mathomatic->rhs[i][j].token.variable > last_v
				    && (v == -1 || mathomatic->rhs[i][j].token.variable < v))
					v = mathomatic->rhs[i][j].token.variable;
			}
		}
		if (v == -1) {
			break;
		}
		last_v = v;
		if (vc) {
			found = false;
			for (j = 0; j < vc; j++) {
				if (v == va[j])
					found = true;
			}
			if (!found)
				continue;
			if (*cp) {
				if (strcmp_tospace(cp, "with") != 0) {
					return false;
				}
				cp1 = skip_param(cp);
				mathomatic->input_column += (cp1 - cp_start);
				if ((cp1 = parse_expr(mathomatic, mathomatic->tes, &mathomatic->n_tes, cp1, true)) == NULL || mathomatic->n_tes <= 0) {
					return false;
				}
				goto do_this;
			}
		}
		list_var(mathomatic, v, 0);
		snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("Enter %s: "), mathomatic->var_str);
		if (!get_expr(mathomatic, mathomatic->tes, &mathomatic->n_tes)) {
			continue;
		}
		value_entered = true;
do_this:
		/* Disguise all variables in the entered expression by making them negative; */
		/* That way they won't be improperly substituted later, allowing variable interchange. */
		for (j = 0; j < mathomatic->n_tes; j += 2) {
			if (mathomatic->tes[j].kind == VARIABLE) {
				mathomatic->tes[j].token.variable = -mathomatic->tes[j].token.variable;
			}
		}
		subst_var_with_exp(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs, mathomatic->tes, mathomatic->n_tes, v);
		subst_var_with_exp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs, mathomatic->tes, mathomatic->n_tes, v);
	}
	/* Restore disguised variables: */
	for (j = 0; j < mathomatic->n_tlhs; j += 2)
		if (mathomatic->tlhs[j].kind == VARIABLE && mathomatic->tlhs[j].token.variable < 0)
			mathomatic->tlhs[j].token.variable = -mathomatic->tlhs[j].token.variable;
	for (j = 0; j < mathomatic->n_trhs; j += 2)
		if (mathomatic->trhs[j].kind == VARIABLE && mathomatic->trhs[j].token.variable < 0)
			mathomatic->trhs[j].token.variable = -mathomatic->trhs[j].token.variable;
	if (mathomatic->repeat_flag) {
		calc_simp(mathomatic, mathomatic->tlhs, &mathomatic->n_tlhs);
		if (mathomatic->n_trhs) {
			calc_simp(mathomatic, mathomatic->trhs, &mathomatic->n_trhs);
			if (se_compare(mathomatic, mathomatic->tlhs, mathomatic->n_tlhs, mathomatic->trhs, mathomatic->n_trhs, &diff_sign) && !diff_sign) {
				fprintf(mathomatic->gfp, _("The result is an identity:\n"));
			}
		}
		list_tdebug(mathomatic, -10);
		if (value_entered) {
			fprintf(mathomatic->gfp, "Repeating:\n");
			goto replace_again;
		} else {
			return true;
		}
	}
	mathomatic->n_lhs[i] = mathomatic->n_tlhs;
	blt(mathomatic->lhs[i], mathomatic->tlhs, mathomatic->n_tlhs * sizeof(token_type));
	mathomatic->n_rhs[i] = mathomatic->n_trhs;
	blt(mathomatic->rhs[i], mathomatic->trhs, mathomatic->n_trhs * sizeof(token_type));
	simp_equation(mathomatic, i);
	return return_result(mathomatic, i);
}

/*
 * The simplify command.
 *
 * Returns number of expressions simplified.
 */
int
simplify_cmd(MathoMatic* mathomatic, char *cp)
{
	int		i, i1;
	int		first, last;
	int		k, k1, total_number_of_solutions, number_simplified = 0;
	long		counter, counter_max, previous_solution_number[N_EQUATIONS];
	sign_array_type	sa_mark, sa_value;
	int		sign_flag = false, quick_flag = false, quickest_flag = false, symb = false, frac_flag = false;
	char		*cp1;

	for (;; cp = skip_param(cp)) {
		if (strncasecmp(cp, "sign", 4) == 0) {
			sign_flag = true;
			continue;
		}
		if (strncasecmp(cp, "symbolic", 4) == 0) {
			symb = true;
			continue;
		}
		if (strcmp_tospace(cp, "quickest") == 0) {
			quickest_flag = true;
			continue;
		}
		if (strcmp_tospace(cp, "quick") == 0) {
			quick_flag = true;
			continue;
		}
		if (strncasecmp(cp, "fraction", 4) == 0) {
			frac_flag = true;
			continue;
		}
		break;
	}
	do {
		cp1 = cp;
		if (!get_range(mathomatic, &cp, &first, &last)) {
			return false;
		}
		if (*cp && cp == cp1) {
			error(mathomatic, _("Invalid argument.  Expecting equation number or range."));
			return false;
		}
		for (i = first; i <= last; i++) {
			if (mathomatic->n_lhs[i] <= 0)
				continue;
			number_simplified++;
			mathomatic->symb_flag = symb;
			if (quickest_flag) {
				simp_equation(mathomatic, i);
			} else {
				simpa_repeat(mathomatic, i, quick_flag, frac_flag);
			}
			mathomatic->symb_flag = false;
			if (!return_result(mathomatic, i)) {
				return false;
			}
			if (!sign_flag)
				continue;
			/* Now substitute all sign variables with +1 and -1. */
			CLEAR_ARRAY(previous_solution_number);
			CLEAR_ARRAY(sa_mark);
			for (k1 = 0; k1 < mathomatic->n_lhs[i]; k1 += 2) {
				if (mathomatic->lhs[i][k1].kind == VARIABLE && (mathomatic->lhs[i][k1].token.variable & VAR_MASK) == SIGN) {
					sa_mark[(mathomatic->lhs[i][k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
			for (k1 = 0; k1 < mathomatic->n_rhs[i]; k1 += 2) {
				if (mathomatic->rhs[i][k1].kind == VARIABLE && (mathomatic->rhs[i][k1].token.variable & VAR_MASK) == SIGN) {
					sa_mark[(mathomatic->rhs[i][k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
			for (k1 = 0, k = 0; k1 < ARR_CNT(sa_mark); k1++) {
				if (sa_mark[k1]) {
					k++;
				}
			}
			if (k == 0)
				continue;
			counter_max = (1L << k) - 1L;
			if (counter_max) {
				fprintf(mathomatic->gfp, _("There are %ld possible solutions.\n"), counter_max + 1);
			}
			for (counter = 0; counter <= counter_max; counter++) {
				i1 = next_espace(mathomatic);
				copy_espace(mathomatic, i, i1);
				for (k1 = 0, k = 0; k1 < ARR_CNT(sa_mark); k1++) {
					if (sa_mark[k1]) {
						sa_value[k1] = (((1L << k) & counter) != 0);
						k++;
					}
				}
				for (k1 = 0; k1 < mathomatic->n_lhs[i1]; k1 += 2) {
					if (mathomatic->lhs[i1][k1].kind == VARIABLE && (mathomatic->lhs[i1][k1].token.variable & VAR_MASK) == SIGN) {
						if (sa_value[(mathomatic->lhs[i1][k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK]) {
							mathomatic->lhs[i1][k1].kind = CONSTANT;
							mathomatic->lhs[i1][k1].token.constant = -1.0;
						} else {
							mathomatic->lhs[i1][k1].kind = CONSTANT;
							mathomatic->lhs[i1][k1].token.constant = 1.0;
						}
					}
				}
				for (k1 = 0; k1 < mathomatic->n_rhs[i1]; k1 += 2) {
					if (mathomatic->rhs[i1][k1].kind == VARIABLE && (mathomatic->rhs[i1][k1].token.variable & VAR_MASK) == SIGN) {
						if (sa_value[(mathomatic->rhs[i1][k1].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK]) {
							mathomatic->rhs[i1][k1].kind = CONSTANT;
							mathomatic->rhs[i1][k1].token.constant = -1.0;
						} else {
							mathomatic->rhs[i1][k1].kind = CONSTANT;
							mathomatic->rhs[i1][k1].token.constant = 1.0;
						}
					}
				}
				for (k1 = 0, k = false; k1 < ARR_CNT(sa_mark); k1++) {
					if (sa_mark[k1]) {
						if (k) {
							fprintf(mathomatic->gfp, ", ");
						} else {
							fprintf(mathomatic->gfp, _("Solution number %ld with "), counter + 1);
						}
						list_var(mathomatic, (long) SIGN + (((long) k1) << VAR_SHIFT), 0);
						fprintf(mathomatic->gfp, "%s = ", mathomatic->var_str);
						if (sa_value[k1]) {
							fprintf(mathomatic->gfp, "-1");
						} else {
							fprintf(mathomatic->gfp, "1");
						}
						k = true;
					}
				}
				if (k)
					fprintf(mathomatic->gfp, ":\n");
				mathomatic->symb_flag = symb;
				if (quickest_flag) {
					simp_equation(mathomatic, i1);
				} else {
					simpa_repeat(mathomatic, i1, quick_flag, frac_flag);
				}
				mathomatic->symb_flag = false;
				for (k1 = 0; k1 < ARR_CNT(previous_solution_number); k1++) {
					if (previous_solution_number[k1]) {
						if (compare_es(mathomatic, k1, i1) > 0) {
							mathomatic->n_lhs[i1] = 0;
							mathomatic->n_rhs[i1] = 0;
							fprintf(mathomatic->gfp, _("is identical to solution number %ld.\n"), previous_solution_number[k1]);
							break;
						}
					}
				}
				if (mathomatic->n_lhs[i1]) {
					list_sub(mathomatic, i1);
					previous_solution_number[i1] = counter + 1;
				}
			}
			total_number_of_solutions = 0;
			for (k1 = 0; k1 < ARR_CNT(previous_solution_number); k1++) {
				if (previous_solution_number[k1]) {
					total_number_of_solutions++;
				}
			}
			if (total_number_of_solutions > 0) {
				number_simplified += total_number_of_solutions;
				fprintf(mathomatic->gfp, _("%d unique solutions stored in equation spaces for this expression (#%d).\n"), total_number_of_solutions, i + 1);
			}
		}
	} while (*cp);
	return number_simplified;
}

/*
 * The factor command.
 */
int
factor_cmd(MathoMatic* mathomatic, char *cp)
{
	int	first, last;
	int	i1;
	int	found, rv = true;
	long	v;				/* Mathomatic variable */
	int	valid_range = false, power_flag = false;
	char	*cp_start;
	int	count_down;
	char	*cp1, *cp2;
	double	d, ed;
#if	!LIBRARY
	char	buf[MAX_CMD_LEN];
#endif

	cp_start = cp;
	if (strcmp_tospace(cp, "number") == 0) {
		cp = skip_param(cp);
	} else if (strcmp_tospace(cp, "numbers") == 0) {
		mathomatic->repeat_flag = true;
		cp = skip_param(cp);
	} else {
		if (strcmp_tospace(cp, "power") == 0) {
			power_flag = true;
			cp = skip_param(cp);
		}
		valid_range = get_range(mathomatic, &cp, &first, &last);
		if (!valid_range) {
#if	LIBRARY	/* be consistent */
			return false;
#else		/* be helpful */
			if (*cp == '-' || isdigit(*cp)) {
				reset_error(mathomatic);
				printf(_("Factoring integers on command-line instead:\n"));
				mathomatic->point_flag = false;
			} else {
				extra_characters(mathomatic, cp);
				return false;
			}
#endif
		}
	}
	if (!valid_range) {
#if	LIBRARY
		mathomatic->repeat_flag = false;
#endif
		do {
			if (*cp == '\0') {
retry:
#if	LIBRARY
				return false;
#else
				my_strlcpy(mathomatic->prompt_str, _("Enter integers to factor: "), sizeof(mathomatic->prompt_str));
				cp = get_string(mathomatic, buf, sizeof(buf));
				if (cp == NULL)
					return false;
				cp_start = cp;
#endif
			}
			if (*cp == '\0')
				return true;
			rv = true;
			for (; *cp; ) {
				cp1 = cp = skip_space(cp);
				errno = 0;
				ed = d = strtod(cp, &cp);
				if (cp == cp1 || errno || !isfinite(d)) {
					goto try_parsing;
				}
				cp = skip_space(cp);
				if (*cp && !isdigit(*cp)) {
					if (*cp == '-') {
						cp2 = cp = skip_space(++cp);
						errno = 0;
						ed = strtod(cp, &cp);
						if (cp == cp2 || errno || !isfinite(ed) || (*cp && *cp != ',' && !isspace(*cp))) {
							goto try_parsing;
						}
					} else {
try_parsing:
						mathomatic->input_column += (cp1 - cp_start);
						cp = parse_expr(mathomatic, mathomatic->tes, &mathomatic->n_tes, cp1, false);
						if (cp == NULL)
							goto retry;
						cp_start = cp;
						if (mathomatic->n_tes <= 0)
							return rv;
						calc_simp(mathomatic, mathomatic->tes, &mathomatic->n_tes);
						if (mathomatic->n_tes != 1 || mathomatic->tes[0].kind != CONSTANT || !isfinite(mathomatic->tes[0].token.constant)) {
							error(mathomatic, _("Integer expected."));
							goto retry;
						}
						ed = d = mathomatic->tes[0].token.constant;
					}
				}
				cp = skip_comma_space(cp);
				count_down = (ed < d);
				for (; count_down ? (d >= ed) : (d <= ed); count_down ? (d -= 1.0) : (d += 1.0)) {
					if (d == 0) {
						fprintf(mathomatic->gfp, _("0 can be evenly divided by any number.\n"));
						continue;
					}
					if (!factor_one(mathomatic, d)) {
						error(mathomatic, _("Number too large to factor or not an integer."));
						rv = false;
						break;
					}
#if	!SILENT
					if (is_prime(mathomatic) && mathomatic->debug_level >= 0) {
						fprintf(mathomatic->gfp, _("Prime number: "));
					}
#endif
					if (!display_unique(mathomatic))
						rv = false;
				}
			}
		} while (mathomatic->repeat_flag);
		return rv;
	}
	if (power_flag) {
		if (extra_characters(mathomatic, cp))
			return false;
		for (i1 = first; i1 <= last; i1++) {
			if (mathomatic->n_lhs[i1]) {
/*				factor_power(lhs[i1], &n_lhs[i1]); */
				do {
					simp_loop(mathomatic, mathomatic->lhs[i1], &mathomatic->n_lhs[i1]);
				} while (factor_power(mathomatic, mathomatic->lhs[i1], &mathomatic->n_lhs[i1]));
				if (mathomatic->n_rhs[i1]) {
/*					factor_power(rhs[i1], &n_rhs[i1]); */
					do {
						simp_loop(mathomatic, mathomatic->rhs[i1], &mathomatic->n_rhs[i1]);
					} while (factor_power(mathomatic, mathomatic->rhs[i1], &mathomatic->n_rhs[i1]));
				}
				if (!return_result(mathomatic, i1))
					return false;
			}
		}
	} else {
		do {
			v = 0;
			if (*cp) {
				if ((cp = parse_var2(mathomatic, &v, cp)) == NULL) {
					return false;
				}
			}
			if (v) {
				found = false;
				for (i1 = first; i1 <= last; i1++) {
					if (var_in_equation(mathomatic, i1, v)) {
						found = true;
						break;
					}
				}
				if (!found) {
					warning(mathomatic, _("Specified variable not found."));
				}
			}
			for (i1 = first; i1 <= last; i1++) {
#if	0
				if (v == 0) {
					if (mathomatic->n_lhs[i1]) {
						simp_loop(mathomatic, mathomatic->lhs[i1], &mathomatic->n_lhs[i1]);
						poly_factor(mathomatic, mathomatic->lhs[i1], &mathomatic->n_lhs[i1], true);
						if (mathomatic->n_rhs[i1]) {
							simp_loop(mathomatic, mathomatic->rhs[i1], &mathomatic->n_rhs[i1]);
							poly_factor(mathomatic, mathomatic->rhs[i1], &mathomatic->n_rhs[i1], true);
						}
					}
				}
#endif
				simpv_equation(mathomatic, i1, v);
			}
		} while (*cp);
		for (i1 = first; i1 <= last; i1++) {
			if (mathomatic->n_lhs[i1]) {
				if (!return_result(mathomatic, i1))
					return false;
			}
		}
	}
	return true;
}

/*
 * Display the number of additive terms in the specified equation space.
 *
 * Return the total number of terms.
 */
int
display_term_count(MathoMatic* mathomatic, int en)
//int	en;	/* equation space number */
{
	int	left_count = 0, right_count = 0;

	if (empty_equation_space(mathomatic, en))
		return 0;
	left_count = level1_plus_count(mathomatic, mathomatic->lhs[en], mathomatic->n_lhs[en]) + 1;
	if (mathomatic->n_rhs[en]) {
		right_count = level1_plus_count(mathomatic, mathomatic->rhs[en], mathomatic->n_rhs[en]) + 1;
		fprintf(mathomatic->gfp, "#%d: LHS consists of %d term%s; ", en + 1, left_count, (left_count == 1) ? "" : "s");
		fprintf(mathomatic->gfp, "RHS consists of %d term%s.\n", right_count, (right_count == 1) ? "" : "s");
	} else {
		fprintf(mathomatic->gfp, "#%d: ", en + 1);
	}
	fprintf(mathomatic->gfp, "Expression consists of a total of %d term%s.\n", left_count + right_count, ((left_count + right_count) == 1) ? "" : "s");
	return(left_count + right_count);
}

/*
 * The unfactor command.
 */
int
unfactor_cmd(MathoMatic* mathomatic, char *cp)
{
	int	first, last;
	int	k;
	int	quick_flag = false, fraction_flag = false, power_flag = false, count_flag = false;

	for (;; cp = skip_param(cp)) {
		if (strncasecmp(cp, "quick", 4) == 0) {
			quick_flag = true;
			continue;
		}
		if (strncasecmp(cp, "fraction", 4) == 0 || strncasecmp(cp, "fully", 4) == 0) {
			fraction_flag = true;
			continue;
		}
		if (strncasecmp(cp, "power", 4) == 0) {
			power_flag = true;
			continue;
		}
		if (strncasecmp(cp, "count", 4) == 0) {
			count_flag = true;
			continue;
		}
		break;
	}
	if (!get_range_eol(mathomatic, &cp, &first, &last)) {
		return false;
	}
	mathomatic->partial_flag = !fraction_flag;
	if (power_flag) {
		for (k = first; k <= last; k++) {
			if (mathomatic->n_lhs[k] <= 0)
				continue;
			if (quick_flag) {
				uf_power(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
			} else {
				uf_allpower(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
			}
			elim_loop(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
			if (mathomatic->n_rhs[k]) {
				if (quick_flag) {
					uf_power(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				} else {
					uf_allpower(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				}
				elim_loop(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
			}
			if (!return_result(mathomatic, k)) {
				mathomatic->partial_flag = true;
				return false;
			}
			if (count_flag) {
				display_term_count(mathomatic, k);
			}
		}
	} else {
		for (k = first; k <= last; k++) {
			if (mathomatic->n_lhs[k] <= 0)
				continue;
			if (quick_flag) {
				uf_tsimp(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
				if (mathomatic->n_rhs[k]) {
					uf_tsimp(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				}
			} else {
				uf_simp(mathomatic, mathomatic->lhs[k], &mathomatic->n_lhs[k]);
				if (mathomatic->n_rhs[k]) {
					uf_simp(mathomatic, mathomatic->rhs[k], &mathomatic->n_rhs[k]);
				}
			}
			if (!return_result(mathomatic, k)) {
				mathomatic->partial_flag = true;
				return false;
			}
			if (count_flag) {
				display_term_count(mathomatic, k);
			}
		}
	}
	mathomatic->partial_flag = true;
	return true;
}

int
div_loc_find(MathoMatic* mathomatic, token_type *expression, int n)
{
	int	k, div_loc;
	int	level;

	level = min_level(mathomatic, expression, n);
	for (k = 1, div_loc = -1; k < n; k += 2) {
		if (expression[k].level == level && expression[k].token.operatr == DIVIDE) {
			if (div_loc >= 0) {
				error_bug(mathomatic, "Expression not grouped.");
			}
			div_loc = k;
		}
	}
	return div_loc;
}

/*
 * The fraction command.
 */
int
fraction_cmd(MathoMatic* mathomatic, char *cp)
{
	int	i, div_loc;
	int	first, last;
	int	num_flag = false, den_flag = false, was_fraction;

	for (;; cp = skip_param(cp)) {
		if (strncasecmp(cp, "numerator", 3) == 0) {
			num_flag = true;
			continue;
		}
		if (strncasecmp(cp, "denominator", 3) == 0) {
			den_flag = true;
			continue;
		}
		break;
	}
	if (!get_range_eol(mathomatic, &cp, &first, &last)) {
		return false;
	}
	mathomatic->show_usage = false;
	for (i = first; i <= last; i++) {
		if (mathomatic->n_lhs[i]) {
			was_fraction = false;
			simple_frac_repeat_side(mathomatic, mathomatic->lhs[i], &mathomatic->n_lhs[i]);
			div_loc = div_loc_find(mathomatic, mathomatic->lhs[i], mathomatic->n_lhs[i]);
			if (div_loc > 0) {
				was_fraction = true;
				if (num_flag && !den_flag) {
					mathomatic->n_lhs[i] = div_loc;
				} else if (den_flag && !num_flag) {
					blt(&mathomatic->lhs[i][0], &mathomatic->lhs[i][div_loc+1], (mathomatic->n_lhs[i] - (div_loc + 1)) * sizeof(token_type));
					mathomatic->n_lhs[i] -= (div_loc + 1);
				}
			}
			if (mathomatic->n_rhs[i]) {
				simple_frac_repeat_side(mathomatic, mathomatic->rhs[i], &mathomatic->n_rhs[i]);
				div_loc = div_loc_find(mathomatic, mathomatic->rhs[i], mathomatic->n_rhs[i]);
				if (div_loc > 0) {
					was_fraction = true;
					if (num_flag && !den_flag) {
						mathomatic->n_rhs[i] = div_loc;
					} else if (den_flag && !num_flag) {
						blt(&mathomatic->rhs[i][0], &mathomatic->rhs[i][div_loc+1], (mathomatic->n_rhs[i] - (div_loc + 1)) * sizeof(token_type));
						mathomatic->n_rhs[i] -= (div_loc + 1);
					}
				}
			}
			if ((num_flag || den_flag) && !was_fraction) {
				warning(mathomatic, _("Expression is not an algebraic fraction."));
				if (den_flag) {
					error(mathomatic, _("Could not extract denominator."));
					return false;
				}
			}
			if (!return_result(mathomatic, i))
				return false;
		}
	}
	return true;
}

#if	!LIBRARY
/*
 * The quit command.
 */
int
quit_cmd(MathoMatic* mathomatic, char *cp)
{
	int	ev = 0;

	if (*cp) {
		ev = decstrtol(cp, &cp);
		if (extra_characters(mathomatic, cp))
			return false;
	}
	exit_program(ev);
	return false;		/* to placate the C compiler */
}
#endif

#if	!SECURE
/*
 * The read command.
 */
int
read_cmd(MathoMatic* mathomatic, char *cp)
{
	int	rv;

	if (mathomatic->security_level >= 3) {
		mathomatic->show_usage = false;
		error(mathomatic, _("Command disabled by security level."));
		return false;
	}
	if (!mathomatic->repeat_flag || *cp == '\0') {
		return read_file(mathomatic, cp);
	}
	do {
		rv = read_file(mathomatic, cp);
	} while (rv);
	return rv;
}

/*
 * Main read command code.
 *
 * Return true if successful.
 */
int
read_file(MathoMatic* mathomatic, char *cp)
{
	int	rv;
	FILE	*fp;
	char	buf[MAX_CMD_LEN];
#if	SHELL_OUT
	char	cl[MAX_CMD_LEN];
	int	ev;	/* exit value */
	char	*lister;
#endif

	if (*cp == '\0') {
#if	SHELL_OUT
#if	MINGW
		lister = "dir /W/P";
#else
		lister = "ls -C";
#endif
		if (mathomatic->gfp && mathomatic->gfp_filename && mathomatic->gfp_filename[0]) {
			if (snprintf(cl, sizeof(cl), "%s >%s%s", lister, mathomatic->gfp_append_flag ? ">" : "", mathomatic->gfp_filename) >= sizeof(cl)) {
				error(mathomatic, _("Command-line too long."));
				return false;
			}
			clean_up(mathomatic);	/* end any redirection */
		} else {
			if (snprintf(cl, sizeof(cl), "%s", lister) >= sizeof(cl)) {
				error(mathomatic, _("Command-line too long."));
				return false;
			}
		}
#if	!MINGW
		printf(_("Listing contents of "));
		output_current_directory(mathomatic, stdout);
		printf("\n");
#endif
		if ((ev = shell_out(mathomatic, cl))) {
			error(mathomatic, _("Error executing directory lister."));
			printf(_("Decimal exit value = %d, shell command-line = %s\n"), ev, cl);
			return false;
		}
		return true;
#else
		error(mathomatic, _("No file name specified."));
		return false;
#endif
	}
	if (snprintf(buf, sizeof(buf), "%s.in", cp) >= sizeof(buf)) {
		error(mathomatic, _("File name too long."));
		return false;
	}
	fp = fopen(buf, "r");
	if (fp == NULL) {
		buf[strlen(cp)] = '\0';
		fp = fopen(buf, "r");
		if (fp == NULL) {
			if (chdir(buf)) {
				error(mathomatic, _("Can't open requested file to read or change directory to."));
				return false;
			} else {
				printf(_("Current working directory changed to "));
				return output_current_directory(mathomatic, stdout);
			}
		}
	}
	rv = read_sub(mathomatic, fp, buf);
	mathomatic->show_usage = false;
	if (fclose(fp)) {
		perror(buf);
		rv = 1;
	}
	if (rv == 100)
		return(true);
#if	!SILENT
	if (!mathomatic->quiet_mode) {
		if (rv) {
			if (!mathomatic->demo_mode) {
				printf(_("Reading of script file \"%s\" aborted due to failure return status\n"), buf);
				printf(_("of a command or expression parsing, or some other error listed above.\n"));
			}
		} else if (mathomatic->debug_level >= 0) {
			printf(_("Successfully finished reading script file \"%s\".\n"), buf);
		}
	}
#endif
	return(!rv);
}

/*
 * Read and process Mathomatic input from a file pointer.
 *
 * Return zero if no error, non-zero if read aborted.
 */
int
read_sub(MathoMatic* mathomatic, FILE *fp, char *filename)
//FILE	*fp;		/* open Mathomatic input file */
//char	*filename;	/* filename of fp */
{
	int	rv;
	jmp_buf	save_save;
	char	*cp;
	int	something_there = false;

	if (fp == NULL) {
		return -1;
	}
	blt(save_save, mathomatic->jmp_save, sizeof(mathomatic->jmp_save));
	if ((rv = setjmp(mathomatic->jmp_save)) != 0) {	/* trap errors */
		clean_up(mathomatic);
		if (rv == 14) {
			error(mathomatic, _("Expression too large."));
		}
		mathomatic->previous_return_value = 0;
	} else {
		while ((cp = fgets((char *) mathomatic->tlhs, mathomatic->n_tokens * sizeof(token_type), fp)) != NULL) {
			if (*cp) {
				something_there = true;
			}
			if (!display_process(mathomatic, cp)) {
				longjmp(mathomatic->jmp_save, 3);	/* jump to the above error trap */
			}
		}
		if (!something_there) {
			if (chdir(filename)) {
				error(mathomatic, _("Empty file (no script to read)."));
				rv = 1;
			} else {
				printf(_("Current directory changed to "));
				output_current_directory(mathomatic, stdout);
				rv = 100;
			}
		}
	}
	blt(mathomatic->jmp_save, save_save, sizeof(mathomatic->jmp_save));
	return rv;
}
#endif

#if	SHELL_OUT
static int
edit_sub(MathoMatic* mathomatic, char *cp)
{
	char	cl[MAX_CMD_LEN];	/* command-line */
	char	*cp1;
	int	ev;	/* exit value */

edit_again:
	cp1 = getenv("EDITOR");
	if (cp1 == NULL) {
#if	CYGWIN || MINGW
		cp1 = "notepad";
#else
		cp1 = "nano";
#endif
		warning(mathomatic, "EDITOR environment variable not set; using default text editor.");
	}
	if (snprintf(cl, sizeof(cl), "%s %s", cp1, cp) >= sizeof(cl)) {
		error(mathomatic, _("Editor command-line too long."));
		return false;
	}
	if ((ev = shell_out(mathomatic, cl))) {
		error(mathomatic, "Error executing editor, check EDITOR environment variable.");
		printf(_("Decimal exit value = %d, shell command-line = %s\n"), ev, cl);
		return false;
	}
	clear_all(mathomatic);
	if (!read_cmd(mathomatic, cp)) {
		if (pause_cmd(mathomatic, _("Prepare to rerun the editor, or type \"quit\""))) {
			goto edit_again;
		}
	}
	return true;
}

/*
 * The edit command.
 */
int
edit_cmd(MathoMatic* mathomatic, char *cp)
{
	FILE	*fp;
#if	!MINGW
	int	fd;
#endif
	int	rv;
	char	tmp_file[MAX_CMD_LEN];

	mathomatic->show_usage = false;
	if (mathomatic->security_level) {
		if (mathomatic->security_level < 0) {
			error(mathomatic, _("Running the editor is not possible with m4."));
		} else {
			error(mathomatic, _("Command disabled by security level."));
		}
		return false;
	}
	clean_up(mathomatic);	/* end any redirection */
	if (*cp == '\0') {
#if	MINGW
		my_strlcpy(tmp_file, "mathomatic.tmp", sizeof(tmp_file));
		fp = fopen(tmp_file, "w+");
		if (fp == NULL) {
			perror(tmp_file);
			error(mathomatic, _("Can't create temporary file."));
			return false;
		}
#else
		my_strlcpy(tmp_file, TMP_FILE, sizeof(tmp_file));
		fd = mkstemp(tmp_file);
		if (fd < 0 || (fp = fdopen(fd, "w+")) == NULL) {
			perror(tmp_file);
			error(mathomatic, _("Can't create temporary file."));
			return false;
		}
#endif
		mathomatic->gfp = fp;
		mathomatic->high_prec = true;
		list_cmd(mathomatic, "all");
		mathomatic->high_prec = false;
		mathomatic->gfp = mathomatic->default_out;
		rv = !ferror(fp);
		if (fclose(fp) || !rv) {
			rv = false;
			perror(tmp_file);
			error(mathomatic, _("Writing temporary file failed."));
		} else {
			rv = edit_sub(mathomatic, tmp_file);
		}
		if (unlink(tmp_file)) {
			perror(tmp_file);
		}
		return rv;
	} else {
		mathomatic->show_usage = true;
		if (access(cp, R_OK | W_OK)) {
			perror(cp);
			error(mathomatic, _("You can only edit existing/writable files or all equation spaces."));
			return false;
		}
		return edit_sub(mathomatic, cp);
	}
}
#endif

#if	!SECURE
/*
 * The save command.
 */
int
save_cmd(MathoMatic* mathomatic, char *cp)
{
	FILE	*fp;
	int	rv, space_flag = false, error_flag;
	char	*cp1;

	if (mathomatic->security_level >= 2) {
		mathomatic->show_usage = false;
		error(mathomatic, _("Command disabled by security level."));
		return false;
	}
	clean_up(mathomatic);	/* end any redirection */
	if (*cp == '\0') {
		error(mathomatic, _("No file name specified; nothing was saved."));
		return false;
	}
	for (cp1 = cp; *cp1; cp1++) {
		if (isspace(*cp1)) {
			space_flag = true;
		}
	}
#if	!SILENT
	if (access(cp, F_OK) == 0) {
		if (access(cp, W_OK)) {
			perror(cp);
			error(mathomatic, _("Specified save file is not writable; choose a different file name."));
			return false;
		}
		snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("File \"%s\" exists, overwrite (y/n)? "), cp);
		if (!get_yes_no(mathomatic)) {
			error(mathomatic, _("File not overwritten; nothing was saved."));
			return false;
		}
	} else if (space_flag) {
		snprintf(mathomatic->prompt_str, sizeof(mathomatic->prompt_str), _("File name \"%s\" contains space characters, create anyways (y/n)? "), cp);
		if (!get_yes_no(mathomatic)) {
			error(mathomatic, _("Save command aborted; nothing was saved."));
			return false;
		}
	}
#endif
	fp = fopen(cp, "w");
	if (fp == NULL) {
		perror(cp);
		error(mathomatic, _("Cannot create specified save file; nothing was saved."));
		return false;
	}
	mathomatic->gfp = fp;
	mathomatic->high_prec = true;
	rv = list_cmd(mathomatic, "all");
	mathomatic->high_prec = false;
	mathomatic->gfp = mathomatic->default_out;
	error_flag = ferror(fp);
	if (fclose(fp) || error_flag) {
		rv = false;
		perror(cp);
	}
	if (rv) {
#if	!SILENT
		printf(_("All expressions saved in file \"%s\".\n"), cp);
#endif
	} else {
		error(mathomatic, _("Error encountered while saving expressions."));
	}
	return rv;
}
#endif
