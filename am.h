/*
 * Main include file for Mathomatic.  Can be edited.
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

#ifndef	DBL_DIG
#error "DBL_DIG not defined!"
#elif DBL_DIG != 15
#warning "DBL_DIG (the number of digits precision of doubles) is not 15."
#warning "This might be a problem."
#endif

#if	sun
#define	INFINITY_NAME	"Infinity"	/* set this to the name of the infinity constant as displayed by printf(3) */
#define NAN_NAME	"NaN"		/* set this to the name of the NaN constant as displayed by printf(3) */
#else
#define	INFINITY_NAME	"inf"	/* set this to the name of the infinity constant as displayed by printf(3) */
#define NAN_NAME	"nan"	/* set this to the name of the NaN constant as displayed by printf(3) */
#endif

#ifndef PATH_MAX					/* make sure the maximum file pathname length is set */
#define PATH_MAX	4096
#endif

#define	MAX_K_INTEGER	1.0e15		/* maximum safely representable floating point integer, 15 digits for doubles */

#define always_positive(power)	(fmod((double) (power), 2.0) == 0.0)	/* true if all real numbers raised to "power" result in positive, real numbers */

#if	I18N		/* Internationalization: allow output in languages other than English using gettext(3). */
#define _(str)		gettext(str)
#else
#define _(str)		str		/* Constant strings to be translated should be marked with this macro. */
					/* Don't bother marking debug and fatal bug error strings. */
#endif

#define	STANDARD_SCREEN_COLUMNS	80			/* default number of columns of characters on the screen */
#define	STANDARD_SCREEN_ROWS	24			/* default number of lines on the screen */
#define	TEXT_ROWS	STANDARD_SCREEN_ROWS		/* number of lines per page in the symbolic math library */
#define	TEXT_COLUMNS	STANDARD_SCREEN_COLUMNS		/* default number of columns per page in the symbolic math library */

#define	TMP_FILE	"/tmp/mathomatic.XXXXXX"	/* temporary file template for mkstemp(3) */

#define	PROMPT_STR	"-> "				/* user interface main prompt strings, preceded by the current equation number */
#define	HTML_PROMPT_STR	"&minus;&gt; "			/* main prompt in HTML output mode, should be same number of columns as above */

#define	MAX_CMD_LEN	min((max(PATH_MAX, 1024)), 16384)	/* Maximum Mathomatic main prompt command-line length */
								/* (not expression input, which can be much larger); */
								/* also max filename length. */
#define	MAX_PROMPT_LEN	STANDARD_SCREEN_COLUMNS		/* maximum length of prompts, limited to fit on normal terminals */

/*
 * What follows is the definition of each array element in a mathematical expression,
 * as stored internally by Mathomatic.
 * Each mathematical expression is stored in a fixed-size array of token_type.
 */

enum kind_list {		/* kinds of expression tokens specified in the union below */
	CONSTANT,
	VARIABLE,
	OPERATOR
};

typedef union {
	double	constant;	/* internal storage for Mathomatic numerical constants */
	long	variable;	/* internal storage for Mathomatic variables */
/* Predefined special variables follow (order is important): */
#define	V_NULL		0L	/* null variable, should never be stored in an expression */
#define	V_E		1L	/* the symbolic, universal constant "e" or "e#" */
#define	V_PI		2L	/* the symbolic, universal constant "pi" or "pi#" */
#define	IMAGINARY	3L	/* the imaginary constant "i" or "i#" */
#define	SIGN		4L	/* for two-valued "sign" variables, numeric variables should be before this */
#define	MATCH_ANY	5L	/* match any variable (wild-card variable) */
#define	V_INTEGER_PREFIX	"integer"	/* prefix for the integer variable type */
	int	operatr;	/* internal storage for Mathomatic operators */
/* Valid operators follow (in precedence order), 0 is reserved for no operator: */
#define	PLUS		1	/* a + b */
#define	MINUS		2	/* a - b */
#define	NEGATE		3	/* special parser operator not seen outside parser */
#define	TIMES		4	/* a * b */
#define	DIVIDE		5	/* a / b */
#define	MODULUS		6	/* a % b */
#define IDIVIDE		7	/* a // b */
#define	POWER		8	/* a ^ b */
#define	FACTORIAL	9	/* a! */
} storage_type;

typedef	struct {		/* storage structure for each token in an expression */
	enum kind_list	kind;	/* kind of token */
	int		level;	/* level of parentheses, origin 1 */
	storage_type	token;	/* the actual token */
} token_type;

/*
 * The following defines the maximum number of equation spaces that can be allocated.
 * The equation spaces are not allocated unless they are used or skipped over.
 * This affects maximum memory usage.
 */
#ifndef	N_EQUATIONS
#define	N_EQUATIONS	200
#endif

/*
 * The following defines the default maximum mathematical expression size.
 * Expression arrays are allocated with this size by default,
 * there are 2 of these for each equation space (1 for LHS and 1 for RHS).
 * DEFAULT_N_TOKENS is linearly related to the actual memory usage of Mathomatic.
 * This should be made much smaller for handhelds and embedded systems.
 * Do not set to less than 100.
 */
#ifndef	DEFAULT_N_TOKENS
#if	HANDHELD
#define	DEFAULT_N_TOKENS	10000
#else
#define	DEFAULT_N_TOKENS	60000
#endif
#endif
#if	(DEFAULT_N_TOKENS < 100 || DEFAULT_N_TOKENS >= (INT_MAX / 3))
#error DEFAULT_N_TOKENS out of range!
#endif

#define	DIVISOR_SIZE		min((DEFAULT_N_TOKENS / 2), 15000)	/* a nice maximum divisor size */

/*
 * All Mathomatic variables are referenced by the value in a C long int variable.
 * The actual name string is stored separately.
 */
#define	MAX_VAR_NAMES	8000			/* maximum number of variable names, keep this under (VAR_MASK - VAR_OFFSET) */
#define	MAX_VAR_LEN	100			/* maximum number of characters in variable names */

#define	MAX_VARS	min(DEFAULT_N_TOKENS / 4, 1000)	/* maximum number of unique variables handled in each equation */

#define	VAR_OFFSET	'A'			/* makes space for predefined variables */
#define	VAR_MASK	0x3fffL			/* mask for bits containing a reference to the variable name */
#define	VAR_SHIFT	14			/* number of bits set in VAR_MASK */
#define	SUBSCRIPT_MASK	63			/* mask for variable subscript after shifting VAR_SHIFT */
#define	MAX_SUBSCRIPT	(SUBSCRIPT_MASK - 1)	/* maximum variable subscript, currently only used for "sign" variables */

typedef	char	sign_array_type[MAX_SUBSCRIPT+2];	/* boolean array for generating unique "sign" variables */

typedef struct {		/* qsort(3) data structure for sorting Mathomatic variables */
	long	v;		/* Mathomatic variable */
	int	count;		/* number of times the variable occurs */
} sort_type;

/* A list of supported output languages for the code command: */
enum language_list {
	C = 1,      /* or C++ */
	JAVA = 2,
	PYTHON = 3
};

/* Debugging macros: */
#if	SILENT
#define	list_esdebug(level, en)		{ ; }	/* Display an equation space. */
#define	list_tdebug(level)		{ ; }	/* Display the temporary equation (e.g.: when solving). */
#define	side_debug(level, p1, n1)	{ ; }	/* Display any expression. */
#define debug_string(level, str)	{ ; }	/* Display any ASCII string. */
#else
#define	list_esdebug(level, en)		list_debug(level, lhs[en], n_lhs[en], rhs[en], n_rhs[en])
#define list_tdebug(level)		list_debug(level, tlhs, n_tlhs, trhs, n_trhs)
#define	side_debug(level, p1, n1)	list_debug(level, p1, n1, NULL, 0)
#define debug_string(level, str)	{ if (debug_level >= (level)) fprintf(gfp, "%s\n", str); }
#endif

/* The correct ways to determine if equation number "en" (origin 0) contains an expression or equation. */
#define empty_equation_space(en)	((en) < 0 || (en) >= n_equations || n_lhs[(en)] <= 0)
#define	equation_space_is_equation(en)	((en) >= 0 && (en) < n_equations && n_lhs[(en)] > 0 && n_rhs[(en)] > 0)

/*
 * The following are macros for displaying help text.
 * When used properly, they will allow nicer looking wrap-around on all help text.
 * If NOT80COLUMNS is 1, paragraphs will all be one long line.
 * This is helpful when the output device has less than 80 text character columns.
 */
#if	NOT80COLUMNS
#define SP(str)	fprintf(gfp, "%s ", str)	/* display part of a paragraph, separated with spaces (Space Paragraph) */
#define EP(str)	fprintf(gfp, "%s\n", str)	/* display the end of a paragraph (End Paragraph) */
#else	/* Otherwise the following only works nicely with 80 column or wider screens; */
	/* all strings passed to it should be less than 80 columns if possible, so it doesn't wrap around. */
#define SP(str)	fprintf(gfp, "%s\n", str)
#define EP(str)	fprintf(gfp, "%s\n", str)
#endif
