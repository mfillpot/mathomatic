/*
 * Mathomatic global variable extern definitions, from file "globals.c".
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

#ifndef MATHO_EXTERNS_H
#define MATHO_EXTERNS_H

typedef struct {
	int		n_tokens;           /* maximum size of expressions, must only be set during startup */
	int		n_equations;        /* number of equation spaces allocated */
	int		cur_equation;       /* current equation space number (origin 0) */

        /* expression storage pointers and current length variables (they go together) */
	token_type	*lhs[N_EQUATIONS];  /* The Left Hand Sides of equation spaces */
	token_type	*rhs[N_EQUATIONS];  /* The Right Hand Sides of equation spaces */

	int		n_lhs[N_EQUATIONS]; /* number of tokens in each lhs[], 0 means equation space is empty */
	int		n_rhs[N_EQUATIONS]; /* number of tokens in each rhs[], 0 means not an equation */

	token_type	*tlhs;              /* LHS during solve and temporary storage for expressions, quotient for poly_div() and smart_div(). */
	token_type	*trhs;              /* RHS during solve and temporary storage for expressions, remainder for poly_div() and smart_div(). */
	token_type	*tes;               /* Temporary Equation Side, used in commands, simpa_repeat_side(), simple_frac_repeat_side(), etc. */
	token_type	*scratch;           /* Very temporary storage for expressions, used only in low level routines for expression manipulation. */

	int		n_tlhs;             /* number of tokens in tlhs */
	int		n_trhs;             /* number of tokens in trhs */
	int		n_tes;              /* number of tokens in tes */


	token_type	zero_token;         /* the universal constant 0.0 as an expression */
	token_type	one_token;          /* the universal constant 1.0 as an expression */

        /* Set options with their initial values. */
	int		precision;              /* the display precision for doubles (number of digits) */
	int		case_sensitive_flag;    /* "set case_sensitive" flag */
	int		factor_int_flag;        /* factor integers when displaying expressions */
	int		display2d;              /* "set no display2d" to allow feeding the output to the input */
	int		fractions_display;      /* "set fraction" mode */
	int		preserve_surds;         /* set option to preserve roots like (2^.5) */
	int		rationalize_denominators;   /* try to rationalize denominators if true */
	int		modulus_mode;           /* true for mathematically correct modulus */
	volatile int	screen_columns;         /* screen width of the terminal; 0 = infinite */
	volatile int	screen_rows;            /* screen height of the terminal; 0 = infinite */
	int		finance_option;         /* for displaying dollars and cents */
	int		autosolve;              /* Allows solving by typing the variable name at the main prompt */
	int		autocalc;               /* Allows automatically calculating a numerical expression */
	int		autodelete;             /* Automatically deletes the previous calculated numerical expression when a new one is entered */
	int		autoselect;             /* Allows selecting equation spaces by typing the number */
	char		special_variable_characters[256];   /* user defined characters for variable names, '\0' terminated */
                                                            /* allow backslash in variable names for Latex compatibility */
	char		plot_prefix[256];       /* prefix fed into gnuplot before the plot command */
	int		factor_out_all_numeric_gcds;    /* if true, factor out the GCD of rational coefficients */
	int		right_associative_power;        /* if true, evaluate power operators right to left */
	int		power_starstar;                 /* if true, display power operator as "**", otherwise "^" */
	#if	!SILENT
	int		debug_level;            /* current debug level */
	#endif
	int		domain_check;
	int		color_flag;             /* "set color" flag; 0 for no color, 1 for color, 2 for alternative color output mode */
	int		bold_colors;            /* bold_colors must be 0 or 1; 0 is dim */
	int		text_color;             /* Current normal text color, -1 for no color */
	int		cur_color;              /* memory of current color on the terminal */
	int		html_flag;              /* 1 for HTML mode on all standard output; 2 for HTML mode on all output, even redirected output */
        
        /* double precision floating point epsilon constants for number comparisons for equivalency */
	double		small_epsilon;          /* for ignoring small, floating point round-off errors */
	double		epsilon;                /* for ignoring larger, accumulated round-off errors */

        /* string variables */
	char		*prog_name;                 /* name of this program */
	char		*var_names[MAX_VAR_NAMES];  /* index for storage of variable name strings */
	char		var_str[MAX_VAR_LEN+80];    /* temp storage for listing a variable name */
	char		prompt_str[MAX_PROMPT_LEN]; /* temp storage for the prompt string */
	#if	!SECURE
	char		rc_file[MAX_CMD_LEN];       /* pathname for the set options startup file */
	#endif

	#if	CYGWIN || MINGW
	char		*dir_path;                  /* directory path to the executable */
	#endif
	#if	READLINE || EDITLINE
	char		*last_history_string;       /* To prevent repeated, identical entries.  Must not point to temporary string. */
	#endif
	#if	READLINE
	char		*history_filename;
	char		history_filename_storage[MAX_CMD_LEN];
	#endif

        /* The following are for integer factoring (filled by factor_one()): */
	double		unique[64];    /* storage for the unique prime factors */
	int		ucnt[64];      /* number of times the factor occurs */
	int		uno;        /* number of unique factors stored in unique[] */

        /* misc. variables */
	int		previous_return_value;  /* Return value of last command entered. */
	sign_array_type	sign_array;             /* for keeping track of unique "sign" variables */
	FILE		*default_out;           /* file pointer where all gfp output goes by default */
	FILE		*gfp;                   /* global output file pointer, for dynamically redirecting Mathomatic output */
	char		*gfp_filename;          /* filename associated with gfp if redirection is happening */
	int		gfp_append_flag;        /* true if appending to gfp, false if overwriting */
	jmp_buf		jmp_save;               /* for setjmp(3) to longjmp(3) to when an error happens deep within this code */
	int		eoption;                /* -e option flag */
	int		test_mode;              /* test mode flag (-t) */
	int		demo_mode;              /* demo mode flag (-d), don't load rc file or pause commands when true */
	int		quiet_mode;             /* quiet mode (-q, don't display prompts) */
	int		echo_input;             /* if true, echo input */
	int		readline_enabled;       /* set to false (-r) to disable readline */
	int		partial_flag;           /* normally true for partial unfactoring, false for "unfactor fraction" */
	int		symb_flag;              /* true during "simplify symbolic", which is not 100% mathematically correct */
	int		symblify;               /* if true, set symb_flag when helpful during solving, etc. */
	int		high_prec;              /* flag to output constants in higher precision (used when saving equations) */
	int		input_column;           /* current column number on the screen at the beginning of a parse */
	int		sign_cmp_flag;          /* true when all "sign" variables are to compare equal */
	int		approximate_roots;      /* true if in calculate command (force approximation of roots like (2^.5)) */  
	volatile int	abort_flag;             /* if true, abort current operation; set by control-C interrupt */
	int		pull_number;            /* equation space number to pull when using the library */
	int		security_level;         /* current enforced security level for session, -1 for m4 Mathomatic */
	int		repeat_flag;            /* true if the command is to repeat its function or simplification, set by repeat command */
	int		show_usage;             /* show command usage info if a command fails and this flag is true */
	int		point_flag;             /* point to location of parse error if true */

        /* library variables go here */
	char		*result_str;            /* returned result text string when using as library */
	int		result_en;              /* equation number of the returned result, if stored in an equation space */
	const char	*error_str;             /* last error string */
	const char	*warning_str;           /* last warning string */

        /* Screen character array, for buffering page-at-a-time 2D string output: */
	char		*vscreen[TEXT_ROWS];
	int		current_columns;

/* Global variables for the optimize command. */
        int	opt_en[N_EQUATIONS+1];
        int	last_temp_var;
/* The following data is used to factor integers: */
        double nn, sqrt_value;
        
        /* from solve.c */
        int	repeat_count;
        int	prev_n1, prev_n2;
        int	last_int_var;
        
        /* from help.c */
        int last_autocalc_en;
        
        /* from integrate.c */
        int constant_var_number;	/* makes unique numbers for the constant of integration */
        
        /* from list.c */
        int cur_line;	/* current line */
        int cur_pos;	/* current position in the current line on the screen */

        /* from poly.c*/
/*
 * The following static expression storage areas are of non-standard size
 * and must only be used for temporary storage.
 * Most Mathomatic expression manipulation and simplification routines should not be used
 * on non-standard or constant size expression storage areas.
 * Standard size expression storage areas that may be
 * manipulated or simplified are the equation spaces, tlhs[], trhs[], and tes[] only.
 */
        token_type	divisor[DIVISOR_SIZE];		/* static expression storage areas for polynomial and smart division */
        int		n_divisor;			/* length of expression in divisor[] */
        token_type	quotient[DIVISOR_SIZE];
        int		n_quotient;			/* length of expression in quotient[] */
        token_type	gcd_divisor[DIVISOR_SIZE];	/* static expression storage area for polynomial GCD routine */
        int		len_d;				/* length of expression in gcd_divisor[] */

} MathoMatic;

//extern MathoMatic *mathomatic;

#endif /*MATHO_EXTERNS_H*/
