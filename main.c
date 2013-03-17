/*
 * This file contains main() and the startup code for Mathomatic,
 * which is a computer algebra system written in the C programming language.
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

/*
 * Output to stderr is only done in this file.  The rest of the Mathomatic code
 * should not output to stderr; error messages should use error() or go to stdout.
 * One reason for this is so that Mathomatic stdout can be redirected or piped,
 * catching all output.
 *
 * This program only supports binary and unary operators.
 * Unary operators are implemented as a binary operation with a dummy operand.
 *
 * In the storage format, each level of parentheses is indicated
 * by a level number (origin 1).  The deeper the level, the
 * higher the level number.
 *
 * The storage format for expressions is a fixed size array of elements
 * "token_type", which may be a CONSTANT, VARIABLE, or OPERATOR.
 * The array always alternates between operand (CONSTANT or VARIABLE)
 * and OPERATOR.  There is a separate integer for each array which
 * contains the current length of the expression stored in the array.
 * This length is always odd and must never exceed "n_tokens".
 *
 * In the storage format,
 * any number of TIMES and DIVIDE operators may be on the same level of parentheses,
 * because they are similar and the most basic multiplicative class operators.
 * The same for the PLUS and MINUS operators, because they are similar (additive class).
 * All other operators are only allowed one single operator per level of parentheses,
 * and no same nor different operators may be with it on that level within the current grouping.
 *
 * Most of the expression manipulation and comparison routines are recursive,
 * calling themselves for each level of parentheses.
 *
 * Note that equation space numbers internally are 1 less than the equation
 * space numbers displayed.  That is because internal equation space numbers
 * are origin 0 array indexes, while displayed equation numbers are origin 1.
 *
 * See the file "am.h" to start understanding the Mathomatic code and
 * to adjust memory usage.
 *
 * C types "long long" and "long double" are not used at all in Mathomatic,
 * because many architectures and compilers do not support them.  These large C
 * types can be used in the Mathomatic Prime Number Tools though, producing
 * larger prime numbers and quickly on a 64-bit computer.  64 bits is the size
 * of a double, so Mathomatic runs optimally on a 64-bit system.
 */

#if	!LIBRARY	/* This comments out this whole file if compiling as the symbolic math library. */

#include "includes.h"
#if	!NO_GETOPT_H
#include <getopt.h>
#endif

#if	WIN32_CONSOLE_COLORS
#include <windows.h>
#include <wincon.h>

HANDLE	hOut;
#endif

/*
 * Display invocation usage info.
 */
void
usage(fp)
FILE	*fp;
{
	fprintf(fp, _("Mathomatic computer algebra system, version %s\n"), VERSION);
	fprintf(fp, _("Usage: %s [ options ] [ input_files or input ]\n\n"), prog_name);
	fprintf(fp, _("Options:\n"));
	fprintf(fp, _("  -a             Enable alternative color mode.\n"));
	fprintf(fp, _("  -b             Enable bold color mode.\n"));
	fprintf(fp, _("  -c             Toggle color mode.\n"));
	fprintf(fp, _("  -d             Set demo mode (no pausing).\n"));
	fprintf(fp, _("  -e             Process expressions and commands on the command line.\n"));
	fprintf(fp, _("  -h             Display this help and exit.\n"));
	fprintf(fp, _("  -m number      Specify a memory size multiplier.\n"));
	fprintf(fp, _("  -q             Set quiet mode (don't display prompts).\n"));
	fprintf(fp, _("  -r             Disable readline or editline.\n"));
	fprintf(fp, _("  -s level:time  Set enforced security level and max time for user's session.\n"));
	fprintf(fp, _("  -t             Set test mode.  Use when comparing program output.\n"));
	fprintf(fp, _("  -u             Set unbuffered output with input echo.\n"));
	fprintf(fp, _("  -v             Display version number, then exit successfully.\n"));
	fprintf(fp, _("  -w             Wide output mode, sets unlimited width.\n"));
	fprintf(fp, _("  -x             Enable HTML/XHTML output mode.\n"));
	fprintf(fp, _("\nPlease refer to the man page for details (type \"man mathomatic\" in shell).\n"));
}

int
main(argc, argv)
int	argc;
char	**argv;
{
#if	NO_GETOPT_H	/* if no getopt.h is available */
	extern char	*optarg;	/* set by getopt(3) */
	extern int	optind;
#endif

	int		i, rv;
	char		*cp = NULL;
	double		numerator, denominator;
	double		new_size = 0;
	int		aoption = false, coption = false, boption = false, wide_flag = false;
	int		exit_value = 0;
	unsigned int	time_out_seconds = 0;

#if	WIN32_CONSOLE_COLORS
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

#if	I18N
	/* Initialize internationalization so that messages are in the right language. */
	setlocale(LC_ALL, "");
	bindtextdomain(prog_name, LOCALEDIR);
	textdomain(prog_name);
#endif

#if	CYGWIN || MINGW
	dir_path = strdup(dirname_win(argv[0]));	/* set dir_path to this executable's directory */
#endif
	/* initialize the global variables */
	init_gvars();
	default_out = stdout;	/* set default_out to any file you want to redirect output to */
	gfp = default_out;
	get_screen_size();

	/* process command line options */
	while ((i = getopt(argc, argv, "s:abqrtdchuvwxm:e")) >= 0) {
		switch (i) {
		case 's':
			if (optarg) {
				security_level = strtod(optarg, &cp);
#if	SECURE
				if (security_level != 4) {
					fprintf(stderr, _("%s: Already compiled for maximum security (level 4), therefore setting security level ignored.\n"), prog_name);
				}
#endif
			}
			if (optarg == NULL || cp == NULL || (cp == optarg && *cp != ':') || (*cp != '\0' && *cp != ':')) {
				fprintf(stderr, _("%s: Error in setting security level.\n"), prog_name);
				exit(2);
			}
			if (*cp == ':') {
				time_out_seconds = strtol(cp + 1, &cp, 10);
				if (!(*cp == '\0' && time_out_seconds > 0)) {
					fprintf(stderr, _("%s: Error in setting time out seconds.\n"), prog_name);
					exit(2);
				}
			}
#if	!SECURE
			if (time_out_seconds > 0) {
				printf(_("Security level is %d, time out seconds is %d.\n"), security_level, time_out_seconds);
			}
#endif
			break;
		case 'w':
			wide_flag = true;
			break;
		case 'a':
			aoption = true;
			break;
		case 'b':
			boption = true;
			break;
		case 'c':
			coption++;
			break;
		case 'x':
			html_flag = 1;
			wide_flag = true;
			break;
		case 'm':
			if (optarg)
				new_size = strtod(optarg, &cp) * DEFAULT_N_TOKENS;
			if (optarg == NULL || cp == NULL || *cp || new_size <= 0 || new_size >= (INT_MAX / sizeof(token_type))) {
				fprintf(stderr, _("%s: Invalid memory size multiplier specified.\n"), prog_name);
				exit(2);
			}
			n_tokens = (int) new_size;
			break;
		case 'q':
			quiet_mode = true;
			break;
		case 'r':
			readline_enabled = false;
			break;
		case 't':
			readline_enabled = false;
			wide_flag = true;
			test_mode = true;
			break;
		case 'd':
			demo_mode = true;
			break;
		case 'u':
			echo_input = true;
			setbuf(stdout, NULL);	/* make output unbuffered */
			setbuf(stderr, NULL);
			break;
		case 'h':
			usage(stdout);
			exit(0);
		case 'v':
/* Don't be fancy, this may be used to test for existence. */
			printf(_("Mathomatic version %s\n"), VERSION);
			exit(0);
		case 'e':
			eoption = true;
			autoselect = false;
			break;
		default:
			usage(stdout);
			exit(2);
		}
	}
	if (n_tokens < 100 || n_tokens >= (INT_MAX / sizeof(token_type))) {
		fprintf(stderr, _("%s: Standard expression array size %d out of range!\n"), prog_name, n_tokens);
	}
	if (!init_mem()) {
		fprintf(stderr, _("%s: Not enough memory.\n"), prog_name);
		exit(2);
	}
#if	READLINE
	if (readline_enabled) {	/* readline_enabled flag must not change after this */
		if ((cp = getenv("HOME"))) {
#if	MINGW
			snprintf(history_filename_storage, sizeof(history_filename_storage), "%s/matho_history", cp);
#else
			snprintf(history_filename_storage, sizeof(history_filename_storage), "%s/.matho_history", cp);
#endif
			history_filename = history_filename_storage;
		}
		using_history();		/* initialize readline history */
		rl_initialize();		/* initialize readline */
		stifle_history(500);		/* maximum of 500 entries */
		rl_inhibit_completion = true;	/* turn off readline file name completion */
#if	0	/* not 100% tested and this might confuse the user with the -c toggle */
#if	!WIN32_CONSOLE_COLORS
		if (!html_flag) {		/* If doing ANSI color: */
			color_flag = (tigetnum("colors") >= 8);	/* autoset color output mode.  Requires ncurses. */
		}
#endif
#endif
#if	!SECURE
		if (security_level <= 3) {
			read_history(history_filename);	/* restore readline history of previous session */
		}
#endif
	}
#endif
	if (html_flag) {
		printf("<pre>\n");
	}
	if (!test_mode && !quiet_mode && !eoption) {
		display_startup_message(stdout);
	}
	fflush(stdout);
#if	!SECURE
	/* read the user options initialization file */
	if (security_level <= 3 && !test_mode && !demo_mode && !load_rc(true, NULL)) {
		fprintf(stderr, _("%s: Error loading startup set options from \"%s\".\n"), prog_name, rc_file);
		fprintf(stderr, _("Use the \"set no save\" command to startup with the program defaults every time.\n\n"));
	}
#endif
	if (wide_flag) {
		screen_columns = 0;
		screen_rows = 0;
	}
	if (coption & 1) {
		color_flag = !color_flag;
	}
	if (boption) {
		color_flag = 1;
		bold_colors = 1;
	}
	if (color_flag && aoption) {
		color_flag = 2;
	}
	if (test_mode) {
		color_flag = 0;
	} else if (!quiet_mode && !eoption) {
		if (color_flag) {
#if	WIN32_CONSOLE_COLORS
			if (color_flag == 2) {
				printf(_("%s%s color mode enabled"), html_flag ? "HTML" : "ANSI", bold_colors ? " bold" : "");
			} else {
				printf(_("%s%s color mode enabled"), html_flag ? "HTML" : "WIN32 CONSOLE", bold_colors ? " bold" : "");
			}
#else
			printf(_("%s%s color mode enabled"), html_flag ? "HTML" : "ANSI", bold_colors ? " bold" : "");
#endif
			printf(_("; manage by typing \"help color\".\n"));
		} else {
			printf(_("Color mode turned off; manage it by typing \"help color\".\n"));
		}
	}
	fflush(stdout);
	if ((i = setjmp(jmp_save)) != 0) {
		/* for error handling */
		clean_up();
		switch (i) {
		case 14:
			error(_("Expression too large."));
		default:
			printf(_("Operation aborted.\n"));
			break;
		}
		previous_return_value = 0;
		if (eoption)
			exit_value = 1;
	} else {
		if ((rv = set_signals(time_out_seconds))) {
			fprintf(stderr, _("C signal handler setting failed!\n"));
#if	DEBUG
			fprintf(stderr, "Failing signal is \"%s\".\n", strsignal(rv));
#endif
			exit_value = 2;
		}
		if (!f_to_fraction(0.5, &numerator, &denominator)
		    || numerator != 1.0 || denominator != 2.0
		    || !f_to_fraction(1.0/3.0, &numerator, &denominator)
		    || numerator != 1.0 || denominator != 3.0) {
			fprintf(stderr, _("%s: Cannot convert any floating point values to fractions!\n"), prog_name);
			fprintf(stderr, _("Roots will not work properly.\n"));
			exit_value = 2;
		}
		if (max_memory_usage() <= 0) {
			fprintf(stderr, _("%s: Calculated maximum memory usage overflows a long integer!\n"), prog_name);
			exit_value = 2;
		}
		if (eoption) {
			/* process expressions and commands from the command line */
			for (i = optind; i < argc && argv[i]; i++) {
				if (!display_process(argv[i])) {
					exit_value = 1;
				}
			}
		} else {
#if	SECURE
			if (!quiet_mode && !test_mode)
				printf(_("Anything done here is temporary.\n"));
			if (optind < argc) {
				warning(_("File arguments ignored in high security mode."));
			}
#else
			if (!quiet_mode && !test_mode) {
				if (optind < argc) {
					printf(_("Reading in file%s specified on the command line...\n"), (optind == (argc - 1)) ? "" : "s");
				} else {
					if (security_level >= 2) {
						printf(_("Anything done here is temporary.\n"));
					} else {
						printf(_("Anything done here is temporary, unless it is saved or redirected.\n"));
					}
				}
			}
			/* read in files specified on the command line, exit if error */
			for (i = optind; i < argc && argv[i]; i++) {
				if (strcmp(argv[i], "-") == 0) {
					main_io_loop();
				} else if (!read_file(argv[i])) {
					fflush(NULL);	/* flush all output */
					fprintf(stderr, _("Read of file \"%s\" failed.\n"), argv[i]);
					exit_program(1);
				}
			}
#endif
		}
	}
	if (!eoption)
		main_io_loop();		/* main input/output loop */
	exit_program(exit_value);	/* exit Mathomatic, doesn't return */
	return(exit_value);		/* so the compiler doesn't complain */
}

/*
 * Repeatedly read a line of text from standard input and process the expression or command.
 */
void
main_io_loop(void)
{
	char	*cp = NULL;

	for (;;) {
		error_str = NULL;
		warning_str = NULL;
		default_color(false);
		snprintf(prompt_str, sizeof(prompt_str), "%d%s", cur_equation + 1, html_flag ? HTML_PROMPT_STR : PROMPT_STR);
		if ((cp = get_string((char *) tlhs, n_tokens * sizeof(token_type))) == NULL)
			break;
		process(cp);
	}
}

/*
 * All signal(2) initialization goes here.
 * Attach all necessary C signals to their handler functions.
 *
 * Return zero on success, or a non-zero unsettable signal number on error.
 */
int
set_signals(time_out_seconds)
unsigned int	time_out_seconds;
{
	int	rv = 0;

	if (signal(SIGFPE, fphandler) == SIG_ERR)
		rv = SIGFPE;
	if (signal(SIGINT, inthandler) == SIG_ERR)
		rv = SIGINT;
	if (signal(SIGTERM, exithandler) == SIG_ERR)
		rv = SIGTERM;
#if	0	/* Crashes entire shell window when readline is used and SIGHUP received. */
	if (signal(SIGHUP, exithandler) == SIG_ERR)
		rv = SIGHUP;
#endif
#if	UNIX || CYGWIN
	if (signal(SIGWINCH, resizehandler) == SIG_ERR)
		rv = SIGWINCH;
#endif
#if	0	/* Crashes entire shell when readline is used. */
	if (signal(SIGALRM, alarmhandler) == SIG_ERR)
		rv = SIGALRM;
#endif
#if	!MINGW
	if (time_out_seconds > 0) {
		alarm(time_out_seconds);
#if	TIMEOUT_SECONDS
	} else {
		alarm(TIMEOUT_SECONDS);
#endif
	}
#endif
	return rv;
}

/*
 * Floating point exception handler.
 * Floating point exceptions are currently ignored.
 */
void
fphandler(sig)
int	sig;
{
#if	DEBUG
	warning("Floating point exception.");
#endif
}

/*
 * Fancy Control-C (interrupt) signal handler.
 * Interrupts processing and returns to main prompt through a polling mechanism.
 * If it can't, repeated calls terminate this program.
 */
void
inthandler(sig)
int	sig;
{
	abort_flag++;
	switch (abort_flag) {
	case 0:
	case 1:
		/* wait for graceful abort */
		printf(_("\nUser interrupt signal received; three times in a row quits Mathomatic.\n"));
		return;
	case 2:
		printf(_("\nPress Control-C once more to quit program.\n"));
		return;
	default:
		/* abruptly quit this program */
		printf(_("\nRepeatedly interrupted; returning to operating system...\n"));
		exit_program(1);
	}
}

#if	0
/*
 * Alarm signal handler.
 */
void
alarmhandler(sig)
int	sig;
{
	printf(_("\nTimeout, quitting...\n"));
	exit_program(1);
}
#endif

/*
 * Signal handler for proper exiting to the operating system.
 */
void
exithandler(sig)
int	sig;
{
	exit_program(1);
}

#if	UNIX || CYGWIN
/*
 * Window resize signal handler.
 */
void
resizehandler(sig)
int	sig;
{
	if (screen_columns)
		get_screen_size();
}
#endif

/*
 * Properly exit this program and return to the operating system.
 */
void
exit_program(exit_value)
int	exit_value;	/* zero if OK, non-zero indicates error return */
{
	reset_attr();
	if (html_flag) {
		printf("</pre>\n");
	}
#if	READLINE && !SECURE
	if (readline_enabled && security_level <= 3) {
		write_history(history_filename);	/* save readline history */
	}
#endif
	if (exit_value == 0 && !quiet_mode && !eoption && !html_flag) {
		printf(_("ByeBye!! from Mathomatic.\n"));
	}
#if	VALGRIND
	printf("Deallocating all Mathomatic allocated memory for valgrind memory leak checking...\n");
	printf("If you are not using valgrind, please compile without -DVALGRIND.\n");
        free_mem();     /* Free all known memory buffers to check for memory leaks with something like valgrind(1). */
#endif
	exit(exit_value);
}
#endif
