/*
 * Mathomatic expression and equation display routines.
 * Color mode routines, too.
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

#if	WIN32_CONSOLE_COLORS
/* The WIN32_CONSOLE_COLORS code was contributed by Doug Snead for the MinGW C compiler. */
#include <windows.h>
#include <wincon.h> 

#define  FOREGROUND_BLACK	0
#define  FOREGROUND_YELLOW      (FOREGROUND_RED|FOREGROUND_GREEN)  
#define  FOREGROUND_MAGENTA     (FOREGROUND_BLUE|FOREGROUND_RED)   
#define  FOREGROUND_CYAN        (FOREGROUND_BLUE|FOREGROUND_GREEN) 
#define  FOREGROUND_WHITE       (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)

/* MS-Windows color array for coloring mathematical expressions, warnings, and errors. */
static short windows_carray[] = {
    FOREGROUND_GREEN, 
    FOREGROUND_YELLOW,	/* warning text color */
    FOREGROUND_RED, 	/* error text color */
    FOREGROUND_MAGENTA,
    FOREGROUND_BLUE, 
    FOREGROUND_CYAN,
}; 

extern HANDLE hOut;
#endif

/* ANSI terminal color code array for 8 color ANSI; we don't use black or white */
/* because the background may be the same color, so there are only 6 colors here. */
static int	carray[] = {
	32,	/* must be green (default color) */
	33,	/* must be yellow (for warnings) */
	31,	/* must be red (for errors) */
	34,	/* must be blue (for prompts) */
	35,	/* magenta */
	36,	/* cyan */
};

#define	EQUATE_STRING	" = "	/* string displayed between the LHS and RHS of equations */
#define MODULUS_STRING	" % "	/* string displayed for the modulus operator */

static int flist_sub(token_type *p1, int n, int out_flag, char *string, int sbuffer_size, int pos, int *highp, int *lowp);
static int flist_recurse(token_type *p1, int n, int out_flag, char *string, int sbuffer_size, int line, int pos, int cur_level, int *highp, int *lowp);

/* Bright HTML color array. */
/* Used when HTML output and "set color" and "set bold" options are enabled. */
/* Good looking with a dark background. */
static char	*bright_html_carray[] = {
	"#00FF00",	/* must be bright green (default color) */
	"#FFFF00",	/* must be bright yellow (for warnings) */
	"#FF0000",	/* must be bright red (for errors) */
	"#0000FF",	/* must be bright blue (for prompts) */
	"#FF9000",
	"#FF00FF",
	"#00FFFF",
};

/* Dim HTML color array for color HTML output. */
/* Used for HTML output with "set color" and "set no bold" options. */
/* Good looking with a white background. */
static char	*html_carray[] = {
	"green",
	"olive",
	"red",
	"navy",
	"maroon",
	"purple",
	"teal",
};

/*
 * Reset terminal attributes function.
 * Turn color off if color mode is on.
 * Subsequent output will have no color until the next call to set_color().
 */
void
reset_attr(void)
{
	FILE	*fp;

	if (html_flag == 2) {
		fp = gfp;
	} else {
		fp = stdout;
	}
#if	!LIBRARY
	fflush(NULL);	/* flush all output */
#endif
	if (color_flag && cur_color >= 0) {
		if (html_flag) {
			fprintf(fp, "</font>");
		} else {
#if	WIN32_CONSOLE_COLORS
			if (color_flag == 2) {
	                        fprintf(fp, "\033[0m");
			} else {
	                        SetConsoleTextAttribute(hOut, FOREGROUND_WHITE);
			}
#else
                        fprintf(fp, "\033[0m");
#endif
		}
		fflush(fp);
	}
	cur_color = -1;
}

/*
 * Set the current color on the display.
 * Subsequent output will have the color in carray[color % ARR_CNT(carray)] for ANSI color mode.
 * Other color modes work similarly.
 * Range for color is 0 to INT_MAX.
 *
 * Return actual color number displayed or -1 if no color.
 */
int
set_color(color)
int	color;
{
	int	rv = -1;

	if (html_flag != 2 && gfp != stdout) {
		return rv;
	}
	if (color_flag) {
		if (cur_color == color)	/* color already set */
			return rv;
		if (html_flag) {
			if (cur_color >= 0) {
				fprintf(gfp, "</font>");
			}
			if (bold_colors) {
				fprintf(gfp, "<font color=\"%s\">", bright_html_carray[rv = (color % ARR_CNT(bright_html_carray))]);
			} else {
				fprintf(gfp, "<font color=\"%s\">", html_carray[rv = (color % ARR_CNT(html_carray))]);
			}
		} else {
#if	WIN32_CONSOLE_COLORS
			if (color_flag == 2) {
	                        fprintf(gfp, "\033[%d;%dm", bold_colors, carray[rv = (color % ARR_CNT(carray))]);
			} else {
	                        short attr = windows_carray[rv = (color % ARR_CNT(windows_carray))];
        	                if (bold_colors)
                	                 attr |= FOREGROUND_INTENSITY;
				SetConsoleTextAttribute(hOut, attr);
			}
#else
                        fprintf(gfp, "\033[%d;%dm", bold_colors, carray[rv = (color % ARR_CNT(carray))]);
#endif
		}
		cur_color = color;
	}
	return rv;
}

/*
 * Set normal text color for subsequent output.
 */
void
default_color(set_no_color_flag)
int	set_no_color_flag;	/* If true, set no color even if text_color is set.  Normally this would be false. */
{
	if (html_flag != 2 && gfp != stdout) {
		return;
	}
	if (color_flag && cur_color >= 0) {
		if (html_flag) {
			fprintf(gfp, "</font>");
		} else {
#if	WIN32_CONSOLE_COLORS
			if (color_flag == 2) {
				fprintf(gfp, "\033[0m");
			} else {
				SetConsoleTextAttribute(hOut, FOREGROUND_WHITE);
			}
#else
			fprintf(gfp, "\033[0m");
#endif
		}
	}
	cur_color = -1;
	if (text_color >= 0 && !set_no_color_flag) {
		set_color(text_color);
	}
	fflush(gfp);
}

/*
 * Display all possible colors for this color mode.
 *
 * Return true if successful.
 */
int
display_all_colors(void)
{
	int	i, j;

	i = 0;
	default_color(true);
	if (set_color(i) < 0) {
		default_color(false);
		return false;
	}
	do {
		printf("#");
		i++;
		j = set_color(i);
	} while (j > 0);
	default_color(false);
	return(j >= 0);
}

/*
 * Trim the trailing zeros from a string, after the decimal point.
 */
static void
trim_zeros(buf)
char	*buf;
{
	int	j;

	j = strlen(buf) - 1;
	for (; j >= 0; j--) {
		if (buf[j] == '0')
			continue;
		if (buf[j] == '.') {
			if (buf[j+1])
				buf[j+2] = '\0';
		} else {
			break;
		}
	}
}

/*
 * Display the expression or equation stored in equation space "n" in single-line format.
 *
 * Return length (number of screen columns) of output line.
 */
int
list1_sub(n, export_flag)
int	n;		/* equation space number */
int	export_flag;	/* non-zero for exportable format (readable by other math programs) */
			/* 1 for Maxima, 2 for other, 3 for gnuplot, 4 for hexadecimal */
{
	int	len = 0;

	if (empty_equation_space(n))
		return 0;
	if ((export_flag == 0 || export_flag == 4) && !high_prec) {
		len += fprintf(gfp, "#%d: ", n + 1);
	}
	len += list_proc(lhs[n], n_lhs[n], export_flag);
	if (n_rhs[n]) {
		len += fprintf(gfp, EQUATE_STRING);
		len += list_proc(rhs[n], n_rhs[n], export_flag);
	}
	if (export_flag == 1) {
		len += fprintf(gfp, ";");
	}
#if	CYGWIN
	fprintf(gfp, "\r\n");	/* might be redirecting to a Microsoft text file */
#else
	fprintf(gfp, "\n");
#endif
	return len;
}

/*
 * Display the expression or equation stored in equation space "n".
 *
 * Return the total width of the output (number of screen columns)
 * or zero on failure.
 */
int
list_sub(n)
int	n;	/* equation space number */
{
	if (empty_equation_space(n))
		return 0;
	make_fractions_and_group(n);
	if (factor_int_flag) {
		factor_int_equation(n);
	}
	if (display2d) {
		/* display in fraction format */
		return flist_equation(n);
	} else {
		/* display in single-line format */
		return list1_sub(n, false);
	}
}

#if	!SILENT
void
list_debug(level, p1, n1, p2, n2)
int		level;
token_type	*p1;
int		n1;
token_type	*p2;
int		n2;
{
	if (debug_level >= level) {
		if (level >= -2) {
			fprintf(gfp, _("level %d: "), level);
		}
		list_proc(p1, n1, false);
		if (p2 && n2 > 0) {
			fprintf(gfp, EQUATE_STRING);
			list_proc(p2, n2, false);
		}
		fprintf(gfp, "\n");
	}
}
#endif

/*
 * Return the allocated string name of the given Mathomatic variable,
 * or NULL if none.
 *
 * Does not return the actual variable name, use list_var() for that.
 */
char *
var_name(v)
long	v;	/* Mathomatic variable */
{
	char	*cp = NULL;
	long	l;

	l = (labs(v) & VAR_MASK) - VAR_OFFSET;
	if (l >= 0 && l < MAX_VAR_NAMES) {
		cp = var_names[l];
	}
	return cp;
}

/*
 * Convert the passed Mathomatic variable to an ASCII variable name.
 * The ASCII variable name is stored in the global var_str[].
 *
 * Return the length of the variable name string in var_str[].
 * Nothing is displayed.
 *
 * If (lang_code == 0), use standard Mathomatic format.
 * If (lang_code > 0), make variable compatible with the language defined in the enumeration language_list defined in am.h
 * If (lang_code < 0), create an exportable variable name: -1 for Maxima, -2 for other, -3 for gnuplot, -4 for hexadecimal,
 * -5 for mathomatic-only variable format.
 */
int
list_var(v, lang_code)
long	v;		/* variable to convert */
int	lang_code;	/* language code */
{
	int		j;
	int		from_memory = false;
	char		*cp = NULL;

	var_str[0] = '\0';
	switch (labs(v) & VAR_MASK) {
	case V_NULL:
		return(strlen(var_str));
	case SIGN:
		cp = "sign";
		break;
	case IMAGINARY:
		switch (lang_code) {
		case -3:
			cp = "{0,1}";
			break;
		case 0:
		case -4:
		case -2:
			cp = "i";
			break;
		case -5:
			cp = "i#";
			break;
		case -1:
			cp = "%i";
			break;
		case PYTHON:
			cp = "1j";
			break;
		default:
			cp = "1.0i";
			break;
		}
		break;
	case V_E:
		switch (lang_code) {
		case -3:
			cp = "exp(1.0)";
			break;
		case -1:
			cp = "%e";
			break;
		case C:
			cp ="M_E";
			break;
		case JAVA:
			cp = "Math.E";
			break;
		case PYTHON:
			cp = "math.e";
			break;
		case -5:
			cp = "e#";
			break;
		default:
			cp = "e";
			break;
		}
		break;
	case V_PI:
		switch (lang_code) {
		case -1:
			cp = "%pi";
			break;
		case -5:
			cp ="pi#";
			break;
		case C:
			cp = "M_PI";
			break;
		case JAVA:
			cp = "Math.PI";
			break;
		case PYTHON:
			cp = "math.pi";
			break;
		default:
			cp = "pi";
			break;
		}
		break;
	case MATCH_ANY:
		cp = "all";
		break;
	default:
		cp = var_name(v);
		from_memory = true;
		break;
	}
	if (cp) {
		j = (labs(v) >> VAR_SHIFT) & SUBSCRIPT_MASK;
		if (j) {	/* for "sign" variables */
			snprintf(var_str, sizeof(var_str), "%s%d", cp, j - 1);
		} else {
			my_strlcpy(var_str, cp, sizeof(var_str));
		}
	} else {
		my_strlcpy(var_str, "bad_variable", sizeof(var_str));
	}
/* Make valid C type variable if exporting or generating code: */
	if (from_memory) {
		switch (lang_code) {
		case 0:
		case -4:
		case -5:
			break;
		default:
			for (j = 0; var_str[j] && var_str[j] != '('; j++) {
				if (strchr("_[]", var_str[j]) == NULL && !isalnum(var_str[j])) {
					var_str[j] = '_';
				}
			}
			break;
		}
	}
	return(strlen(var_str));
}

/*
 * Display an expression in single-line format.
 * Use color if color mode is set.
 *
 * Return number of characters output (excluding escape sequences).
 */
int
list_proc(p1, n, export_flag)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
int		export_flag;	/* flag for exportable format (usually false) */
				/* 1 for Maxima, 2 for other, 3 for gnuplot, 4 for hexadecimal */
{
	return list_string_sub(p1, n, true, NULL, export_flag);
}

/*
 * Store the expression from the specified equation space in a text string in single-line format.
 * String should be freed with free() when done.
 *
 * Returns text string, or NULL if error.
 */
char *
list_equation(n, export_flag)
int	n;		/* equation space number */
int	export_flag;	/* flag for exportable format (usually false) */
{
	int	len;
	char	*cp;

	if (empty_equation_space(n))
		return NULL;
	len = list_string(lhs[n], n_lhs[n], NULL, export_flag);
	if (n_rhs[n]) {
		len += strlen(EQUATE_STRING);
		len += list_string(rhs[n], n_rhs[n], NULL, export_flag);
	}
	len += 2;	/* for possible semicolon and terminating null character */
	cp = (char *) malloc(len);
	if (cp == NULL) {
		error(_("Out of memory (can't malloc(3))."));
		return NULL;
	}
	list_string(lhs[n], n_lhs[n], cp, export_flag);
	if (n_rhs[n]) {
		strcat(cp, EQUATE_STRING);
		list_string(rhs[n], n_rhs[n], &cp[strlen(cp)], export_flag);
	}
	if (export_flag == 1) {
		strcat(cp, ";");
	}
	return cp;
}

/*
 * Store an expression in a text string.
 * String should be freed with free() when done.
 *
 * Return string, or NULL if error.
 */
char *
list_expression(p1, n, export_flag)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
int		export_flag;
{
	int	len;
	char	*cp;

	if (n <= 0) {
		return NULL;
	}
	len = list_string(p1, n, NULL, export_flag);
	len++;	/* for terminating null character */
	cp = (char *) malloc(len);
	if (cp == NULL) {
		error(_("Out of memory (can't malloc(3))."));
		return NULL;
	}
	list_string(p1, n, cp, export_flag);
	return cp;
}

/*
 * Convert an expression to a text string and store in "string" if
 * "string" is not NULL.  "string" need not be initialized,
 * but must be long enough to contain the expression.
 * To find the storage size needed, call with "string" set to NULL first.
 *
 * Return length (number of characters).
 */
int
list_string(p1, n, string, export_flag)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
char		*string;	/* buffer to save output to or NULL pointer */
int		export_flag;
{
	return list_string_sub(p1, n, false, string, export_flag);
}

#define	APPEND(str)	{ if (string) { strcpy(&string[len], str); } if (outflag) { fprintf(gfp, "%s", str); } len += strlen(str); }
#define	APPEND2(str)	{ if (string) { if ((sbuffer_size - current_len) > 0) my_strlcpy(&string[current_len], str, sbuffer_size - current_len); } else { fprintf(gfp, "%s", str); } current_len += strlen(str); }

int
list_string_sub(p1, n, outflag, string, export_flag)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
int		outflag;	/* if true, output to gfp */
char		*string;	/* buffer to save output to or NULL pointer */
int		export_flag;	/* flag for exportable format (usually false) */
				/* 1 for Maxima, 2 for other, 3 for gnuplot, 4 for hexadecimal */
{
	int	i, j, k, i1;
	int	min1;
	int	cur_level;
	char	*cp;
	int	len = 0;
	char	buf[500], buf2[500];
	int	export_precision;
	int	cflag, power_flag;

	cflag = (outflag && (export_flag == 0 || export_flag == 4));
	if (cflag)
		set_color(0);
	if (string)
		string[0] = '\0';
	if (high_prec)
		export_precision = 20;
	else
		export_precision = DBL_DIG;
	cur_level = min1 = min_level(p1, n);
	for (i = 0; i < n; i++) {
		power_flag = false;
		if (export_flag == 0 && !high_prec) {
			for (j = i - 1; j <= (i + 1); j++) {
				if ((j - 1) >= 0 && (j + 1) < n
				    && p1[j].kind == OPERATOR && (p1[j].token.operatr == POWER || p1[j].token.operatr == FACTORIAL)
				    && p1[j-1].level == p1[j].level
				    && p1[j+1].level == p1[j].level
				    && ((j + 2) >= n || p1[j+2].level != (p1[j].level - 1) || (p1[j+2].token.operatr < POWER))
				    && ((j - 2) < 0 || p1[j-2].level != (p1[j].level - 1) || (p1[j-2].token.operatr < POWER))) {
					power_flag = true;
					break;
				}
			}
		}
		j = cur_level - p1[i].level;
		if (power_flag)
			k = abs(j) - 1;
		else
			k = abs(j);
		for (i1 = 1; i1 <= k; i1++) {
			if (j > 0) {
				cur_level--;
				APPEND(")");
				if (cflag)
					set_color(cur_level-min1);
			} else {
				cur_level++;
				if (cflag)
					set_color(cur_level-min1);
				APPEND("(");
			}
		}
		switch (p1[i].kind) {
		case CONSTANT:
			if (p1[i].token.constant == 0.0) {
				p1[i].token.constant = 0.0; /* fix -0 */
			}
			if (export_flag == 4) {
				snprintf(buf, sizeof(buf), "%a", p1[i].token.constant);
			} else if (export_flag == 3) {
				snprintf(buf, sizeof(buf), "%#.*g", DBL_DIG, p1[i].token.constant);
				trim_zeros(buf);
			} else if (export_flag || high_prec) {
				snprintf(buf, sizeof(buf), "%.*g", export_precision, p1[i].token.constant);
			} else if (finance_option >= 0) {
#if	THOUSANDS_SEPARATOR	/* Fails miserably in MinGW and possibly others, displaying nothing but the format string. */
				snprintf(buf, sizeof(buf), "%'.*f", finance_option, p1[i].token.constant);
#else
				snprintf(buf, sizeof(buf), "%.*f", finance_option, p1[i].token.constant);
#endif
			} else {
				if (p1[i].token.constant < 0.0 && (i + 1) < n && p1[i+1].level == p1[i].level
				    && (p1[i+1].token.operatr >= POWER)) {
					snprintf(buf, sizeof(buf), "(%.*g)", precision, p1[i].token.constant);
				} else {
					snprintf(buf, sizeof(buf), "%.*g", precision, p1[i].token.constant);
				}
				APPEND(buf);
				break;
			}
			if (p1[i].token.constant < 0.0) {
				snprintf(buf2, sizeof(buf2), "(%s)", buf);
				APPEND(buf2);
			} else {
				APPEND(buf);
			}
			break;
		case VARIABLE:
			list_var(p1[i].token.variable, 0 - export_flag);
			APPEND(var_str);
			break;
		case OPERATOR:
			cp = _("(unknown operator)");
			switch (p1[i].token.operatr) {
			case PLUS:
				cp = " + ";
				break;
			case MINUS:
				cp = " - ";
				break;
			case TIMES:
				cp = "*";
				break;
			case DIVIDE:
				cp = "/";
				break;
			case IDIVIDE:
				cp = "//";
				break;
			case MODULUS:
				cp = MODULUS_STRING;
				break;
			case POWER:
				if (power_starstar || export_flag == 3) {
					cp = "**";
				} else {
					cp = "^";
				}
				break;
			case FACTORIAL:
				cp = "!";
				i++;
				break;
			}
			APPEND(cp);
			break;
		}
	}
	for (j = cur_level - min1; j > 0;) {
		APPEND(")");
		j--;
		if (cflag)
			set_color(j);
	}
	if (cflag)
		default_color(false);
	return len;
}

/*
 * Return 1 (true) or -1 if expression is a valid integer expression for
 * list_code().
 * Return 0 if it is definitely a non-integer expression.
 * Return -1 if it contains non-integer divide operators, but is OK otherwise.
 */
int
int_expr(p1, n)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
{
	int	i;
	int	rv = 1;

	for (i = 0; i < n; i++) {
		switch (p1[i].kind) {
		case CONSTANT:
			if (fmod(p1[i].token.constant, 1.0) != 0.0) {
				return 0;
			}
			break;
		case VARIABLE:
			if (p1[i].token.variable < IMAGINARY) {
				return 0;
			}
			break;
		case OPERATOR:
			if (p1[i].token.operatr == DIVIDE)
				rv = -1;
			break;
		}
	}
	return rv;
}

/*
 * Display an equation space as C, Java, or Python code.
 *
 * Return length of output (number of characters).
 */
int
list_code_equation(en, language, int_flag)
int			en;		/* equation space number */
enum language_list	language;
int			int_flag;	/* integer arithmetic flag */
{
	int	len = 0;

	if (empty_equation_space(en))
		return 0;
	len += list_code(lhs[en], &n_lhs[en], true, NULL, language, int_flag);
	if (n_rhs[en]) {
		len += fprintf(gfp, EQUATE_STRING);
		len += list_code(rhs[en], &n_rhs[en], true, NULL, language, int_flag);
	}
	switch (language) {
	case C:
	case JAVA:
		len += fprintf(gfp, ";");
		break;
	default:
		break;
	}
	fprintf(gfp, "\n");
	return len;
}

/*
 * Convert the specified equation space to a string of C, Java, or Python code.
 * String should be freed with free() when done.
 *
 * Return string, or NULL if error.
 */
char *
string_code_equation(en, language, int_flag)
int			en;		/* equation space number */
enum language_list	language;
int			int_flag;	/* integer arithmetic flag */
{
	int	len;
	char	*cp;

	if (empty_equation_space(en))
		return NULL;
	len = list_code(lhs[en], &n_lhs[en], false, NULL, language, int_flag);
	if (n_rhs[en]) {
		len += strlen(EQUATE_STRING);
		len += list_code(rhs[en], &n_rhs[en], false, NULL, language, int_flag);
	}
	len += 2;	/* for possible semicolon and terminating null character */
	cp = (char *) malloc(len);
	if (cp == NULL) {
		error(_("Out of memory (can't malloc(3))."));
		return NULL;
	}
	list_code(lhs[en], &n_lhs[en], false, cp, language, int_flag);
	if (n_rhs[en]) {
		strcat(cp, EQUATE_STRING);
		list_code(rhs[en], &n_rhs[en], false, &cp[strlen(cp)], language, int_flag);
	}
	switch (language) {
	case C:
	case JAVA:
		strcat(cp, ";");
		break;
	default:
		break;
	}
	return cp;
}

/*
 * Output C, Java, or Python code for an expression.
 * Expression might be modified by this function, though it remains equivalent.
 *
 * Return length of output (number of characters).
 */
int
list_code(equation, np, outflag, string, language, int_flag)
token_type		*equation;	/* equation side pointer */
int			*np;		/* pointer to length of equation side */
int			outflag;	/* if true, output to gfp */
char			*string;	/* buffer to save output to or NULL pointer */
enum language_list	language;	/* see enumeration language_list in am.h */
int			int_flag;	/* integer arithmetic flag, should work with any language */
{
	int	i, j, k, i1, i2;
	int	min1;
	int	cur_level;
	char	*cp;
	char	buf[500], buf2[500];
	int	len = 0;

	if (string)
		string[0] = '\0';
	min1 = min_level(equation, *np);
	if (*np > 1)
		min1--;
	cur_level = min1;
	for (i = 0; i < *np; i++) {
		j = cur_level - equation[i].level;
		k = abs(j);
		for (i1 = 1; i1 <= k; i1++) {
			if (j > 0) {
				cur_level--;
				APPEND(")");
			} else {
				cur_level++;
				for (i2 = i + 1; i2 < *np && equation[i2].level >= cur_level; i2 += 2) {
					if (equation[i2].level == cur_level) {
						switch (equation[i2].token.operatr) {
						case POWER:
							if (equation[i2-1].level == cur_level
							    && equation[i2+1].level == cur_level
							    && equation[i2+1].kind == CONSTANT
							    && equation[i2+1].token.constant == 2.0) {
								equation[i2].token.operatr = TIMES;
								equation[i2+1] = equation[i2-1];
							} else {
								if (!int_flag) {
									switch (language) {
									case C:
										APPEND("pow");
							 			break;
									case JAVA:
										APPEND("Math.pow");
										break;
									default:
										break;
									}
								}
							}
							break;
						case FACTORIAL:
							APPEND("factorial");
							break;
						}
						break;
					}
				}
				APPEND("(");
			}
		}
		switch (equation[i].kind) {
		case CONSTANT:
			if (equation[i].token.constant == 0.0) {
				equation[i].token.constant = 0.0; /* fix -0 */
			}
			if (int_flag) {
				snprintf(buf, sizeof(buf), "%.0f", equation[i].token.constant);
			} else {
				snprintf(buf, sizeof(buf), "%#.*g", DBL_DIG, equation[i].token.constant);
				trim_zeros(buf);
			}
/* Here we will need to parenthesize negative numbers to make -2**x work the same with Python: */
			if (equation[i].token.constant < 0) {
				snprintf(buf2, sizeof(buf2), "(%s)", buf);
				APPEND(buf2);
			} else {
				APPEND(buf);
			}
			break;
		case VARIABLE:
		  	if (int_flag && (language == C || language == JAVA) && equation[i].token.variable == IMAGINARY) {
				APPEND("1i");
			} else {
				list_var(equation[i].token.variable, language);
				APPEND(var_str);
			}
			break;
		case OPERATOR:
			cp = _("(unknown operator)");
			switch (equation[i].token.operatr) {
			case PLUS:
				cp = " + ";
				break;
			case MINUS:
				cp = " - ";
				break;
			case TIMES:
				cp = "*";
				break;
			case IDIVIDE:
				if (language == PYTHON) {
					cp = "//";
					break;
				}
			case DIVIDE:
				cp = "/";
				break;
			case MODULUS:
				cp = MODULUS_STRING;
				break;
			case POWER:
				if (int_flag || language == PYTHON) {
					cp = "**";
				} else {
					cp = ", ";
				}
				break;
			case FACTORIAL:
				cp = "";
				i++;
				break;
			}
			APPEND(cp);
			break;
		}
	}
	for (j = cur_level - min1; j > 0; j--) {
		APPEND(")");
	}
	return len;
}

/* global variables for the flist functions below */
static int	cur_line;	/* current line */
static int	cur_pos;	/* current position in the current line on the screen */

/*
 * Return a multi-line C string containing the specified equation space in 2D multi-line fraction format.
 * The string should be freed after use with free(3).
 *
 * Color mode is not used.
 *
 * The equation sides must first be basically simplified and prepared by fractions_and_group(),
 * for proper formatting.
 *
 * Return NULL on failure.
 * Result will be limited to current_columns columns, TEXT_ROWS lines.
 * Exceeding current_columns will result in truncation,
 * exceeding TEXT_ROWS will result in failure and NULL return.
 * current_columns is set in malloc_vscreen().
 */
char *
flist_equation_string(n)
int	n;	/* equation space number */
{
	int	i;
	int	len, cur_len, buf_len;
	int	pos;
	int	high = 0, low = 0;
	int	max_line = 0, min_line = 0;
	int	screen_line;
	char	*cp;

	if (empty_equation_space(n))
		return NULL;
	if (!malloc_vscreen())
		return NULL;
	for (i = 0; i < TEXT_ROWS; i++) {
		vscreen[i][0] = '\0';
	}
	cur_line = 0;
	cur_pos = 0;
	len = flist_sub(lhs[n], n_lhs[n], false, NULL, current_columns, 0, &max_line, &min_line);
	if (n_rhs[n]) {
		len += strlen(EQUATE_STRING);
		len += flist_sub(rhs[n], n_rhs[n], false, NULL, current_columns, 0, &high, &low);
		if (high > max_line)
			max_line = high;
		if (low < min_line)
			min_line = low;
	}
	if ((max_line - min_line) >= TEXT_ROWS)
		return NULL;
	for (cur_line = max_line, screen_line = 0; cur_line >= min_line; cur_line--, screen_line++) {
		pos = cur_pos = 0;
		pos += flist_sub(lhs[n], n_lhs[n], true, vscreen[screen_line], current_columns, pos, &high, &low);
		if (n_rhs[n]) {
			if (cur_line == 0) {
				cur_pos += strlen(EQUATE_STRING);
				cur_len = strlen(vscreen[screen_line]);
				if ((current_columns - cur_len) > 0)
					my_strlcpy(&vscreen[screen_line][cur_len], EQUATE_STRING, current_columns - cur_len);
			}
			pos += strlen(EQUATE_STRING);
			pos += flist_sub(rhs[n], n_rhs[n], true, vscreen[screen_line], current_columns, pos, &high, &low);
		}
	}
	if (screen_line <= 0)
		return NULL;
	for (i = 0, buf_len = 1; i < screen_line; i++) {
		buf_len += strlen(vscreen[i]);
		buf_len++;	/* For newlines */
	}
	cp = (char *) malloc(buf_len);
	if (cp == NULL) {
		error(_("Out of memory (can't malloc(3))."));
		return NULL;
	}
	cp[0] = '\0';
	for (i = 0; i < screen_line; i++) {
		strcat(cp, vscreen[i]);
		strcat(cp, "\n");
	}
	return(cp);
}

/*
 * Display an equation space in 2D multi-line fraction format.
 * Use color if color mode is set.
 *
 * The equation sides must first be basically simplified and prepared by fractions_and_group(),
 * for proper display.
 *
 * Return the total width of the output (that is, the required number of screen columns)
 * or zero on failure.
 */
int
flist_equation(n)
int	n;	/* equation space number */
{
	int	sind;
	char	buf[50];
	int	len, len2, len3, width;
	int	pos;
	int	high = 0, low = 0;
	int	max_line = 0, min_line = 0;
	int	max2_line = 0, min2_line = 0;
	int	use_screen_columns;	/* use infinite width if false */

	if (empty_equation_space(n))
		return 0;
#if	1	/* always respect "set columns" */
	use_screen_columns = true;
#else
	use_screen_columns = (gfp == stdout);
#endif
	len = snprintf(buf, sizeof(buf), "#%d: ", n + 1);
	cur_line = 0;
	cur_pos = 0;
	sind = n_rhs[n];
	len += flist_sub(lhs[n], n_lhs[n], false, NULL, screen_columns, 0, &max_line, &min_line);
	if (n_rhs[n]) {
		len += strlen(EQUATE_STRING);
make_smaller:
		len2 = flist_sub(rhs[n], sind, false, NULL, screen_columns, 0, &high, &low);
		if (screen_columns && use_screen_columns && (len + len2) >= screen_columns && sind > 0) {
			for (sind--; sind > 0; sind--) {
				if (rhs[n][sind].level == 1 && rhs[n][sind].kind == OPERATOR) {
					switch (rhs[n][sind].token.operatr) {
					case PLUS:
					case MINUS:
					case MODULUS:
						goto make_smaller;
					}
				}
			}
			goto make_smaller;
		}
		if (high > max_line)
			max_line = high;
		if (low < min_line)
			min_line = low;
		len3 = flist_sub(&rhs[n][sind], n_rhs[n] - sind, false, NULL, screen_columns, 0, &max2_line, &min2_line);
	} else {
		len2 = 0;
		len3 = 0;
	}
	width = max(len + len2, len3);
	if (screen_columns && use_screen_columns && width >= screen_columns) {
		/* output too wide to fit screen, output in single-line format */
		width = list1_sub(n, false);
#if	CYGWIN
		fprintf(gfp, "\r\n");	/* Be consistent with list1_sub() output. */
#else
		fprintf(gfp, "\n");
#endif
		return width;
	}
	fprintf(gfp, "\n");
	for (cur_line = max_line; cur_line >= min_line; cur_line--) {
		pos = cur_pos = 0;
		if (cur_line == 0) {
			cur_pos += fprintf(gfp, "%s", buf);
		}
		pos += strlen(buf);
		pos += flist_sub(lhs[n], n_lhs[n], true, NULL, screen_columns, pos, &high, &low);
		if (n_rhs[n]) {
			if (cur_line == 0) {
				cur_pos += fprintf(gfp, "%s", EQUATE_STRING);
			}
			pos += strlen(EQUATE_STRING);
			pos += flist_sub(rhs[n], sind, true, NULL, screen_columns, pos, &high, &low);
		}
		fprintf(gfp, "\n");
	}
	if (sind < n_rhs[n]) {
/* output second half of split equation that was too wide to display on the screen without splitting */
		fprintf(gfp, "\n");
		for (cur_line = max2_line; cur_line >= min2_line; cur_line--) {
			cur_pos = 0;
			flist_sub(&rhs[n][sind], n_rhs[n] - sind, true, NULL, screen_columns, 0, &high, &low);
			fprintf(gfp, "\n");
		}
	}
	fprintf(gfp, "\n");
	return width;
}

/*
 * Display a line of an expression in 2D fraction format if "out_flag" is true.
 * The number of the line to output is stored in the global variable cur_line.
 * 0 is the middle line, lines above are positive, lines below are negative.
 * Use color if available.
 *
 * The following functions are for internal use only, and not to be called,
 * except by flist_equation() and flist_equation_string().
 *
 * Return the width of the expression (that is, the required number of screen columns).
 */
static int
flist_sub(p1, n, out_flag, string, sbuffer_size, pos, highp, lowp)
token_type	*p1;		/* expression pointer */
int		n;		/* length of expression */
int		out_flag;	/* if true, output to gfp or string */
char		*string;	/* if not NULL, put output to here instead of gfp */
int		sbuffer_size;	/* string buffer size */
int		pos;
int		*highp, *lowp;
{
	int	rv;

	rv = flist_recurse(p1, n, out_flag, string, sbuffer_size, 0, pos, 1, highp, lowp);
	if (out_flag && (string == NULL)) {
		default_color(false);
	}
	return rv;
}

static int
flist_recurse(p1, n, out_flag, string, sbuffer_size, line, pos, cur_level, highp, lowp)
token_type	*p1;
int		n;
int		out_flag;	/* if true, output to gfp or string */
char		*string;	/* if not NULL, put output to here instead of gfp */
int		sbuffer_size;	/* string buffer size */
int		line;
int		pos;
int		cur_level;
int		*highp, *lowp;
{
	int	i, j, k, i1;
	int	l1, l2;
	int	ii;
	int	stop_at;
	int	div_loc;
	int	len_div;
	int	level;
	int	start_level;
	int	oflag, cflag, html_out, power_flag;
	int	len = 0, len1, len2;
	int	current_len = 0;
	int	high, low;
	char	buf[500];
	char	*cp;

	if (string)
		current_len = strlen(string);
	start_level = cur_level;
	*highp = line;
	*lowp = line;
	if (n <= 0) {
		return 0;
	}
	oflag = (out_flag && line == cur_line);
	cflag = (oflag && string == NULL);
	html_out = ((html_flag == 2) || (html_flag && gfp == stdout));
	if (oflag) {
		for (; cur_pos < pos; cur_pos++) {
			APPEND2(" ");
		}
	}
	ii = 0;
check_again:
	stop_at = n;
	div_loc = -1;
	for (i = ii; i < n; i++) {
		if (p1[i].kind == OPERATOR && p1[i].token.operatr == DIVIDE) {
			level = p1[i].level;
			for (j = i - 2; j > 0; j -= 2) {
				if (p1[j].level < level)
					break;
			}
			j++;
			if (div_loc < 0) {
				div_loc = i;
				stop_at = j;
			} else {
				if (j < stop_at) {
					div_loc = i;
					stop_at = j;
				} else if (j == stop_at) {
					if (level < p1[div_loc].level)
						div_loc = i;
				}
			}
		}
	}
	for (i = ii; i < n; i++) {
		power_flag = false;
		if (i == stop_at) {
#if	DEBUG
			if (div_loc < 0) {
				error_bug("Bug in flist_recurse().");
			}
#endif
			j = cur_level - p1[div_loc].level;
			k = abs(j) - 1;
		} else {
			for (j = i - 1; j <= (i + 1); j++) {
				if ((j - 1) >= ii && (j + 1) < n
				    && p1[j].kind == OPERATOR && (p1[j].token.operatr == POWER || p1[j].token.operatr == FACTORIAL)
				    && p1[j-1].level == p1[j].level
				    && p1[j+1].level == p1[j].level
				    && ((j + 2) >= n || p1[j+2].level != (p1[j].level - 1) || (p1[j+2].token.operatr < POWER))
				    && ((j - 2) < ii || p1[j-2].level != (p1[j].level - 1) || (p1[j-2].token.operatr < POWER))) {
					power_flag = true;
					break;
				}
			}
			j = cur_level - p1[i].level;
			if (power_flag)
				k = abs(j) - 1;
			else
				k = abs(j);
		}
		if (k < 1) {
			if (cflag)
				set_color(cur_level-1);
		}
		for (i1 = 1; i1 <= k; i1++) {
			if (j > 0) {
				cur_level--;
				len++;
				if (oflag) {
					APPEND2(")");
					if (cflag)
						set_color(cur_level-1);
				}
			} else {
				cur_level++;
				len++;
				if (oflag) {
					if (cflag)
						set_color(cur_level-1);
					APPEND2("(");
				}
			}
		}
		if (i == stop_at) {
			level = p1[div_loc].level;
			len1 = flist_recurse(&p1[stop_at], div_loc - stop_at, false, string, sbuffer_size, line + 1, pos + len, level, &high, &low);
			l1 = (2 * (line + 1)) - low;
			for (j = div_loc + 2; j < n; j += 2) {
				if (p1[j].level <= level)
					break;
			}
			len2 = flist_recurse(&p1[div_loc+1], j - (div_loc + 1), false, string, sbuffer_size, line - 1, pos + len, level, &high, &low);
			l2 = (2 * (line - 1)) - high;
			ii = j;
			len_div = max(len1, len2);
			j = 0;
			if (len1 < len_div) {
				j = (len_div - len1) / 2;
			}
			flist_recurse(&p1[stop_at], div_loc - stop_at, out_flag, string, sbuffer_size, l1, pos + len + j, level, &high, &low);
			if (high > *highp)
				*highp = high;
			if (low < *lowp)
				*lowp = low;
			if (oflag) {
				/* display fraction bar */
				if (cflag)
					set_color(level-1);
				for (j = 0; j < len_div; j++) {
					if (html_out) {
						APPEND2("&ndash;");
					} else {
						APPEND2("-");
					}
				}
				if (cflag)
					set_color(cur_level-1);
			}
			j = 0;
			if (len2 < len_div) {
				j = (len_div - len2) / 2;
			}
			flist_recurse(&p1[div_loc+1], ii - (div_loc + 1), out_flag, string, sbuffer_size, l2, pos + len + j, level, &high, &low);
			if (high > *highp)
				*highp = high;
			if (low < *lowp)
				*lowp = low;
			len += len_div;
			goto check_again;
		}
		switch (p1[i].kind) {
		case CONSTANT:
			if (p1[i].token.constant == 0.0) {
				p1[i].token.constant = 0.0; /* fix -0 */
			}
			if (html_out && isinf(p1[i].token.constant)) {
				if (p1[i].token.constant < 0) {
					snprintf(buf, sizeof(buf), "(-&infin;)");
					len += 4;
				} else {
					snprintf(buf, sizeof(buf), "&infin;");
					len += 1;
				}
			} else if (p1[i].token.constant == -1.0 && (i == 0 || p1[i-1].level < p1[i].level)
			    && (i + 1) < n && p1[i].level == p1[i+1].level && p1[i+1].token.operatr == TIMES) {
				i++;
				len += snprintf(buf, sizeof(buf), "-");
			} else if (finance_option >= 0) {
				if (p1[i].token.constant < 0.0) {
#if	THOUSANDS_SEPARATOR
					len += snprintf(buf, sizeof(buf), "(%'.*f)", finance_option, p1[i].token.constant);
#else
					len += snprintf(buf, sizeof(buf), "(%.*f)", finance_option, p1[i].token.constant);
#endif
				} else {
#if	THOUSANDS_SEPARATOR
					len += snprintf(buf, sizeof(buf), "%'.*f", finance_option, p1[i].token.constant);
#else
					len += snprintf(buf, sizeof(buf), "%.*f", finance_option, p1[i].token.constant);
#endif
				}
			} else {
				if (p1[i].token.constant < 0.0 && (i + 1) < n && p1[i+1].level == p1[i].level
				    && (p1[i+1].token.operatr >= POWER)) {
					len += snprintf(buf, sizeof(buf), "(%.*g)", precision, p1[i].token.constant);
				} else {
					len += snprintf(buf, sizeof(buf), "%.*g", precision, p1[i].token.constant);
				}
			}
			if (oflag)
				APPEND2(buf);
			break;
		case VARIABLE:
			if (html_out && p1[i].token.variable == V_PI) {
				len += 1;
				if (oflag)
					APPEND2("&pi;");
			} else if (html_out && p1[i].token.variable == V_E) {
				len += 1;
				if (oflag)
					APPEND2("&ecirc;");
			} else if (html_out && p1[i].token.variable == IMAGINARY) {
				len += 1;
				if (oflag)
					APPEND2("&icirc;");
			} else {
				len += list_var(p1[i].token.variable, 0);
				if (oflag)
					APPEND2(var_str);
			}
			break;
		case OPERATOR:
/* "len" must be incremented here by the number of columns the display of the operator takes */
			switch (p1[i].token.operatr) {
			case PLUS:
				cp = " + ";
				len += 3;
				break;
			case MINUS:
				if (html_out) {
					cp = " &minus; ";
				} else {
					cp = " - ";
				}
				len += 3;
				break;
			case TIMES:
				if (html_out) {
					cp = "&middot;";
				} else {
					cp = "*";
				}
				len++;
				break;
			case DIVIDE:
				cp = "/";
				len++;
				break;
			case IDIVIDE:
				cp = "//";
				len += 2;
				break;
			case MODULUS:
				cp = MODULUS_STRING;
				len += strlen(cp);
				break;
			case POWER:
#if	0
				if (html_out
				    && p1[i+1].level == p1[i].level
				    && p1[i+1].kind == CONSTANT) {
					len += (snprintf(buf, sizeof(buf), "<sup>%.*g</sup>", precision, p1[i+1].token.constant) - 11);
					cp = buf;
					i++;
					break;
				}
#endif
				if (power_starstar) {
					cp = "**";
					len += 2;
				} else {
					cp = "^";
					len++;
				}
				break;
			case FACTORIAL:
				cp = "!";
				len++;
				i++;
				break;
			default:
				cp = _("(unknown operator)");
				len += strlen(cp);
				break;
			}
			if (oflag)
				APPEND2(cp);
			break;
		}
	}
	for (j = cur_level - start_level; j > 0;) {
		cur_level--;
		len++;
		j--;
		if (oflag) {
			APPEND2(")");
			if (j > 0 && cflag)
				set_color(cur_level-1);
		}
	}
	if (oflag)
		cur_pos += len;
	return len;
}
