/*
 * Mathomatic API, Copyright (C) 1996-2012 George Gesslein II.
 *
 * This file contains the required C functions and simple interface
 * for developers to use the Mathomatic symbolic math library properly.
 * These functions are included in the library.
 * Refer to this, if you are going to use the Mathomatic code in other projects.
 *
 * Be sure to call matho_clear(3) to erase all equation spaces
 * after completing each group of operations,
 * otherwise the equation spaces will fill up.
 */

#include "../includes.h"
#include "mathomatic.h"

/** 3
 * matho_init - Initialize the Mathomatic symbolic math library
 * Call this only once before calling any Mathomatic code.
 * This must be called exactly once upon program startup and not again,
 * unless free_mem() is called.
 *
 * Returns true if successful.
 * If this returns false, there was not enough memory available
 * and Mathomatic cannot be used.
 */
int
matho_init(void)
{
	init_gvars();
	default_out = stdout;	/* if default_out is a file that is not stdout, output is logged to that file */
	gfp = default_out;
	if (!init_mem()) {
		return false;
	}
	signal(SIGFPE, fphandler);	/* handle floating point exceptions, currently ignored */
	return true;
}

/** 3
 * matho_clear - Erase all equation spaces so they can be reused
 * Mathomatic only has a limited number of equation spaces.
 * Similar to a restart, recommended after each group of symbolic math operations.
 * Currently this is the same as entering the command "clear all".
 *
 * matho_init(3) must have been called only one time before this
 * to initialize the Mathomatic symbolic math engine.
 */
void
matho_clear(void)
{
	clear_all();
}

/** 3
 * matho_process - Process Mathomatic command or expression input
 * Process a Mathomatic command or enter an expression into an equation space.
 * The command or expression ASCII string is given as "input",
 * the resulting output string is stored in "*outputp".
 *
 * matho_init(3) must have been called only one time before this
 * to initialize the Mathomatic symbolic math engine.
 * Use matho_clear(3) as many times as you want to restart Mathomatic
 * for the next group of operations.
 *
 * This function works just like typing something into the Mathomatic prompt.
 * To only parse any expression or equation and store it, use matho-parse(3).
 *
 * If this returns true (non-zero), the command or input was successful,
 * and the resulting expression output string is stored in "*outputp".
 * That is a malloc()ed text string which must be free()d after use
 * to return the memory used by the string.
 * The equation number of the equation space that the output expression
 * is additionally stored in (if any) is available in the global "result_en",
 * otherwise result_en = -1.
 *
 * If this returns false, the command or input failed and a text error
 * message is always stored in "*outputp".
 * The error message is a constant string and should NOT be free()d.
 *
 * Some commands, like the set command, will return no output when successful,
 * setting "*outputp" to NULL.
 *
 * The resulting output string can safely be ignored by calling
 * this function with "outputp" set to NULL.
 */
int
matho_process(char *input, char **outputp)
{
	int	i;
	int	rv;

	if (outputp)
		*outputp = NULL;
	result_str = NULL;
	result_en = -1;
	error_str = NULL;
	warning_str = NULL;
	if (input == NULL)
		return false;
	input = strdup(input);
	if ((i = setjmp(jmp_save)) != 0) {
		clean_up();	/* Mathomatic processing was interrupted, so do a clean up. */
		if (i == 14) {
			error(_("Expression too large."));
		}
		if (outputp) {
			if (error_str) {
				*outputp = (char *) error_str;
			} else {
				*outputp = _("Processing was interrupted.");
			}
		}
		free_result_str();
		free(input);
		previous_return_value = 0;
		return false;
	}
	set_error_level(input);
	rv = process(input);
	if (rv) {
		if (outputp) {
			*outputp = result_str;
		} else {
			if (result_str) {
				free(result_str);
				result_str = NULL;
			}
		}
	} else {
		if (outputp) {
			if (error_str) {
				*outputp = (char *) error_str;
			} else {
				*outputp = _("Unknown error.");
			}
		}
		free_result_str();
	}
	free(input);
	return rv;
}

/** 3
 * matho_parse - Process Mathomatic expression or equation input
 * Parse a mathematical equation or expression and store in the next available equation space,
 * making it the current equation.
 * Afterwards, it can be operated on by Mathomatic commands using matho_process(3).
 *
 * matho_init(3) must have been called only one time before this
 * to initialize the Mathomatic symbolic math engine.
 * Use matho_clear(3) as many times as you want to restart Mathomatic
 * for the next group of operations.
 *
 * The input and output ASCII strings are expressions, if successful.
 * The expression or equation string to enter is in "input",
 * the resulting output string is stored in "*outputp".
 * The equation number of the equation space that the output expression
 * is additionally stored in (if any) is available in the global "result_en",
 * otherwise result_en = -1.
 *
 * Works the same as matho_process(3), except commands are not allowed,
 * so that variables are not ever confused with commands.
 * In fact, this function is currently set to only allow
 * entry and storage of expressions and equations.
 *
 * Returns true (non-zero) if successful.
 */
int
matho_parse(char *input, char **outputp)
{
	int	i;
	int	rv;

	if (outputp)
		*outputp = NULL;
	result_str = NULL;
	result_en = -1;
	error_str = NULL;
	warning_str = NULL;
	if (input == NULL)
		return false;
	input = strdup(input);
	if ((i = setjmp(jmp_save)) != 0) {
		clean_up();	/* Mathomatic processing was interrupted, so do a clean up. */
		if (i == 14) {
			error(_("Expression too large."));
		}
		if (outputp) {
			if (error_str) {
				*outputp = (char *) error_str;
			} else {
				*outputp = _("Processing was interrupted.");
			}
		}
		free_result_str();
		free(input);
		return false;
	}
	set_error_level(input);
	i = next_espace();
#if	1	/* Leave this as 1 if you want to be able to enter single variable or constant expressions with no solving or selecting. */
	rv = parse(i, input);	/* All set auto options ignored. */
#else
	rv = process_parse(i, input);	/* All set auto options respected. */
#endif
	if (rv) {
		if (outputp) {
			*outputp = result_str;
		} else {
			if (result_str) {
				free(result_str);
				result_str = NULL;
			}
		}
	} else {
		if (outputp) {
			if (error_str) {
				*outputp = (char *) error_str;
			} else {
				*outputp = _("Unknown error.");
			}
		}
		free_result_str();
	}
	free(input);
	return rv;
}

/*
 * Floating point exception handler.
 * Usually doesn't work in most operating systems, so just ignore it.
 */
void
fphandler(int sig)
{
/*	error(_("Floating point exception.")); */
}
