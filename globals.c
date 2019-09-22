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

//MathoMatic *mathomatic;

MathoMatic *newtMathoMatic(void) {
    MathoMatic *mathomatic = malloc(sizeof(MathoMatic));
    if(!mathomatic) return NULL;
    memset(mathomatic, 0, sizeof(MathoMatic));

    mathomatic->n_tokens = DEFAULT_N_TOKENS;	/* maximum size of expressions, must only be set during startup */
    /* Set options with their initial values. */
    mathomatic->precision = 14;				/* the display precision for doubles (number of digits) */
    mathomatic->case_sensitive_flag = true;		/* "set case_sensitive" flag */
    #if	LIBRARY && !ROBOT_COMMAND
    mathomatic->display2d = false;			/* "set no display2d" to allow feeding the output to the input */
    #else
    mathomatic->display2d = true;			/* "set display2d" flag for 2D display */
    #endif
    mathomatic->fractions_display = 1;			/* "set fraction" mode */
    mathomatic->preserve_surds = true;			/* set option to preserve roots like (2^.5) */
    mathomatic->rationalize_denominators = true;	/* try to rationalize denominators if true */
    mathomatic->modulus_mode = 2;				/* true for mathematically correct modulus */
    mathomatic->screen_columns = STANDARD_SCREEN_COLUMNS;	/* screen width of the terminal; 0 = infinite */
    mathomatic->screen_rows = STANDARD_SCREEN_ROWS;		/* screen height of the terminal; 0 = infinite */
    mathomatic->finance_option = -1;				/* for displaying dollars and cents */
    mathomatic->autosolve = true;			/* Allows solving by typing the variable name at the main prompt */
    mathomatic->autocalc = true;			/* Allows automatically calculating a numerical expression */
    mathomatic->autodelete = false;			/* Automatically deletes the previous calculated numerical expression when a new one is entered */
    mathomatic->autoselect = true;			/* Allows selecting equation spaces by typing the number */
    #if	LIBRARY
    strcpy(mathomatic->special_variable_characters, "\\[]");	/* allow backslash in variable names for Latex compatibility */
    #else
    strcpy(mathomatic->special_variable_characters, "'\\[]");	/* user defined characters for variable names, '\0' terminated */
    #endif
    #if	MINGW
    strcpy(mathomatic->plot_prefix, "set grid; set xlabel 'X'; set ylabel 'Y';");		/* prefix fed into gnuplot before the plot command */
    #else
    strcpy(mathomatic->plot_prefix, "set grid; set xlabel \"X\"; set ylabel \"Y\";");	/* prefix fed into gnuplot before the plot command */
    #endif
    mathomatic->factor_out_all_numeric_gcds = false;	/* if true, factor out the GCD of rational coefficients */

    /* variables having to do with color output mode */
    #if	LIBRARY || NO_COLOR
    mathomatic->color_flag = 0;			/* library shouldn't default to color mode */
    #else
    mathomatic->color_flag = 1;			/* "set color" flag; 0 for no color, 1 for color, 2 for alternative color output mode */
    #endif
    #if	BOLD_COLOR
    mathomatic->bold_colors = 1;		/* "set bold color" flag for brighter colors */
    #else
    mathomatic->bold_colors = 0;		/* bold_colors must be 0 or 1; 0 is dim */
    #endif
    mathomatic->text_color = -1;		/* Current normal text color, -1 for no color */
    mathomatic->cur_color = -1;			/* memory of current color on the terminal */

    /* double precision floating point epsilon constants for number comparisons for equivalency */
    mathomatic->small_epsilon	= 0.000000000000005;	/* for ignoring small, floating point round-off errors */
    mathomatic->epsilon		= 0.00000000000005;	/* for ignoring larger, accumulated round-off errors */

    /* string variables */
    mathomatic->prog_name = "mathomatic";	/* name of this program */

    /* misc. variables */
    mathomatic->previous_return_value = 1;	/* Return value of last command entered. */
    mathomatic->readline_enabled = true;	/* set to false (-r) to disable readline */
    mathomatic->symblify = true;	/* if true, set symb_flag when helpful during solving, etc. */

    /* library variables go here */
    mathomatic->result_en = -1;		/* equation number of the returned result, if stored in an equation space */
    
    mathomatic->last_autocalc_en = -1;
    mathomatic->constant_var_number = 1;	/* makes unique numbers for the constant of integration */

    return mathomatic;
}

void closetMathoMatic(MathoMatic *mathomatic) {
    free(mathomatic);
}

int matho_cur_equation(MathoMatic * mathomatic) {
    return mathomatic->cur_equation;
}

int matho_result_en(MathoMatic * mathomatic) {
    return mathomatic->result_en;
}

const char *matho_get_warning_str(MathoMatic * mathomatic) {
    return mathomatic->warning_str;
}

void matho_set_warning_str(MathoMatic * mathomatic, const char *ws) {
    mathomatic->warning_str = ws;
}

void matho_set_error_str(MathoMatic * mathomatic, const char *ws) {
    mathomatic->error_str = ws;
}

int matho_get_abort_flag(MathoMatic* mathomatic) {
    return mathomatic->abort_flag;
}

void matho_inc_abort_flag(MathoMatic* mathomatic) {
    ++mathomatic->abort_flag;
}
