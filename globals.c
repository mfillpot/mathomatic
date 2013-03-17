/*
 * Mathomatic global variables and arrays.
 * Most global variables for Mathomatic are defined here and duplicated in "externs.h".
 *
 * C initializes global variables and arrays to zero by default.
 * This is required for proper operation.
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

int		n_tokens = DEFAULT_N_TOKENS;	/* maximum size of expressions, must only be set during startup */

int		n_equations,			/* number of equation spaces allocated */
		cur_equation;			/* current equation space number (origin 0) */

/* expression storage pointers and current length variables (they go together) */
token_type	*lhs[N_EQUATIONS],		/* The Left Hand Sides of equation spaces */
		*rhs[N_EQUATIONS];		/* The Right Hand Sides of equation spaces */

int		n_lhs[N_EQUATIONS],		/* number of tokens in each lhs[], 0 means equation space is empty */
		n_rhs[N_EQUATIONS];		/* number of tokens in each rhs[], 0 means not an equation */

token_type	*tlhs,				/* LHS during solve and temporary storage for expressions, quotient for poly_div() and smart_div(). */
		*trhs,				/* RHS during solve and temporary storage for expressions, remainder for poly_div() and smart_div(). */
		*tes,				/* Temporary Equation Side, used in commands, simpa_repeat_side(), simple_frac_repeat_side(), etc. */
		*scratch;			/* Very temporary storage for expressions, used only in low level routines for expression manipulation. */
						/* Do not run any functions on scratch[], except for blt() (which is memmove(3)). */

int		n_tlhs,				/* number of tokens in tlhs */
		n_trhs,				/* number of tokens in trhs */
		n_tes;				/* number of tokens in tes */

token_type	zero_token,			/* the universal constant 0.0 as an expression */
		one_token;			/* the universal constant 1.0 as an expression */

/* Set options with their initial values. */
int		precision = 14;				/* the display precision for doubles (number of digits) */
int		case_sensitive_flag = true;		/* "set case_sensitive" flag */
int		factor_int_flag;			/* factor integers when displaying expressions */
#if	LIBRARY && !ROBOT_COMMAND
int		display2d = false;			/* "set no display2d" to allow feeding the output to the input */
#else
int		display2d = true;			/* "set display2d" flag for 2D display */
#endif
int		fractions_display = 1;			/* "set fraction" mode */
int		preserve_surds = true;			/* set option to preserve roots like (2^.5) */
int		rationalize_denominators = true;	/* try to rationalize denominators if true */
int		modulus_mode = 2;				/* true for mathematically correct modulus */
volatile int	screen_columns = STANDARD_SCREEN_COLUMNS;	/* screen width of the terminal; 0 = infinite */
volatile int	screen_rows = STANDARD_SCREEN_ROWS;		/* screen height of the terminal; 0 = infinite */
int		finance_option = -1;				/* for displaying dollars and cents */
int		autosolve = true;			/* Allows solving by typing the variable name at the main prompt */
int		autocalc = true;			/* Allows automatically calculating a numerical expression */
int		autodelete = false;			/* Automatically deletes the previous calculated numerical expression when a new one is entered */
int		autoselect = true;			/* Allows selecting equation spaces by typing the number */
#if	LIBRARY
char		special_variable_characters[256] = "\\[]";	/* allow backslash in variable names for Latex compatibility */
#else
char		special_variable_characters[256] = "'\\[]";	/* user defined characters for variable names, '\0' terminated */
#endif
#if	MINGW
char		plot_prefix[256] = "set grid; set xlabel 'X'; set ylabel 'Y';";		/* prefix fed into gnuplot before the plot command */
#else
char		plot_prefix[256] = "set grid; set xlabel \"X\"; set ylabel \"Y\";";	/* prefix fed into gnuplot before the plot command */
#endif
int		factor_out_all_numeric_gcds = false;	/* if true, factor out the GCD of rational coefficients */
int		right_associative_power;		/* if true, evaluate power operators right to left */
int		power_starstar;				/* if true, display power operator as "**", otherwise "^" */
#if	!SILENT
int		debug_level;				/* current debug level */
#endif

/* variables having to do with color output mode */
#if	LIBRARY || NO_COLOR
int		color_flag = 0;			/* library shouldn't default to color mode */
#else
int		color_flag = 1;			/* "set color" flag; 0 for no color, 1 for color, 2 for alternative color output mode */
#endif
#if	BOLD_COLOR
int		bold_colors = 1;		/* "set bold color" flag for brighter colors */
#else
int		bold_colors = 0;		/* bold_colors must be 0 or 1; 0 is dim */
#endif
int		text_color = -1;		/* Current normal text color, -1 for no color */
int		cur_color = -1;			/* memory of current color on the terminal */
int		html_flag;			/* 1 for HTML mode on all standard output; 2 for HTML mode on all output, even redirected output */

/* double precision floating point epsilon constants for number comparisons for equivalency */
double		small_epsilon	= 0.000000000000005;	/* for ignoring small, floating point round-off errors */
double		epsilon		= 0.00000000000005;	/* for ignoring larger, accumulated round-off errors */

/* string variables */
char		*prog_name = "mathomatic";	/* name of this program */
char		*var_names[MAX_VAR_NAMES];	/* index for storage of variable name strings */
char		var_str[MAX_VAR_LEN+80];	/* temp storage for listing a variable name */
char		prompt_str[MAX_PROMPT_LEN];	/* temp storage for the prompt string */
#if	!SECURE
char		rc_file[MAX_CMD_LEN];		/* pathname for the set options startup file */
#endif

#if	CYGWIN || MINGW
char		*dir_path;			/* directory path to the executable */
#endif
#if	READLINE || EDITLINE
char		*last_history_string;		/* To prevent repeated, identical entries.  Must not point to temporary string. */
#endif
#if	READLINE
char		*history_filename;
char		history_filename_storage[MAX_CMD_LEN];
#endif

/* The following are for integer factoring (filled by factor_one()): */
double		unique[64];		/* storage for the unique prime factors */
int		ucnt[64];		/* number of times the factor occurs */
int		uno;			/* number of unique factors stored in unique[] */

/* misc. variables */
int		previous_return_value = 1;	/* Return value of last command entered. */
sign_array_type	sign_array;		/* for keeping track of unique "sign" variables */
FILE		*default_out;		/* file pointer where all gfp output goes by default */
FILE		*gfp;			/* global output file pointer, for dynamically redirecting Mathomatic output */
char		*gfp_filename;		/* filename associated with gfp if redirection is happening */
int		gfp_append_flag;	/* true if appending to gfp, false if overwriting */
jmp_buf		jmp_save;		/* for setjmp(3) to longjmp(3) to when an error happens deep within this code */
int		eoption;		/* -e option flag */
int		test_mode;		/* test mode flag (-t) */
int		demo_mode;		/* demo mode flag (-d), don't load rc file or pause commands when true */
int		quiet_mode;		/* quiet mode (-q, don't display prompts) */
int		echo_input;		/* if true, echo input */
int		readline_enabled = true;	/* set to false (-r) to disable readline */
int		partial_flag;		/* normally true for partial unfactoring, false for "unfactor fraction" */
int		symb_flag;		/* true during "simplify symbolic", which is not 100% mathematically correct */
int		symblify = true;	/* if true, set symb_flag when helpful during solving, etc. */
int		high_prec;		/* flag to output constants in higher precision (used when saving equations) */
int		input_column;		/* current column number on the screen at the beginning of a parse */
int		sign_cmp_flag;		/* true when all "sign" variables are to compare equal */
int		domain_check;		/* flag to track domain errors in the pow() function */
int		approximate_roots;	/* true if in calculate command (force approximation of roots like (2^.5)) */
volatile int	abort_flag;		/* if true, abort current operation; set by control-C interrupt */
int		pull_number;		/* equation space number to pull when using the library */
int		security_level;		/* current enforced security level for session, -1 for m4 Mathomatic */
int		repeat_flag;		/* true if the command is to repeat its function or simplification, set by repeat command */
int		show_usage;		/* show command usage info if a command fails and this flag is true */
int		point_flag;		/* point to location of parse error if true */

/* library variables go here */
char		*result_str;		/* returned result text string when using as library */
int		result_en = -1;		/* equation number of the returned result, if stored in an equation space */
const char	*error_str;		/* last error string */
const char	*warning_str;		/* last warning string */

/* Screen character array, for buffering page-at-a-time 2D string output: */
char		*vscreen[TEXT_ROWS];
int		current_columns;
