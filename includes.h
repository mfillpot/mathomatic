/*
 * Standard C include files for Mathomatic.
 * Automatically includes all necessary C include files for
 * any Mathomatic C source code.
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

#define	true	1
#define	false	0

#if	0
#define	_REENTRANT	1	/* Can be defined before including math.h for Mac OS X.  Mac OS X allows a few re-entrant functions with this.  iOS requires this commented out. */
#endif

#if	ROBOT_COMMAND
#define NOT80COLUMNS	1	/* For programs that use less than 80 columns wide display. */
#endif

#ifndef HELP
#define HELP	1	/* Define HELP=0 to remove the help command.  Shaves at least 10 kilobytes off of the executable. */
#endif

#if	LIBRARY		/* If compiling this Mathomatic code as the symbolic math library: */
#ifndef	SILENT		/* Define SILENT=0 to have debugging enabled with the symbolic math library. */
#define	SILENT	1	/* Disable debug level setting and stop messages going to stdout. */
#endif
#undef	READLINE	/* Readline shouldn't be included in the library code. */
#undef	EDITLINE	/* Editline neither */
#endif

#if	__CYGWIN__ && !CYGWIN
#warning Compiling under Cygwin without proper defines.
#warning Please define CYGWIN on the compiler command line with -DCYGWIN
#define	CYGWIN	1
#endif

#if 	CYGWIN || MINGW
#undef	UNIX		/* Unix desktop functionality is slightly different for CYGWIN and MINGW */
#endif

#if	(UNIX || CYGWIN || MINGW) && !SECURE && !LIBRARY
#define SHELL_OUT	1	/* include the code to shell out (run system(3) commmand) */
#endif

#if	SECURE && SHELL_OUT
#warning SHELL_OUT defined during secure mode compilation.  This is a security problem.
#endif

/* Include files from /usr/include: */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if	UNIX
#include <libgen.h>
#endif

#if	sun
#include <ieeefp.h>
#endif

#if	SHOW_RESOURCES
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <limits.h>
#include <float.h>
#include <math.h>
#include <setjmp.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#if	I18N		/* Internationalization doesn't work yet.  It would need a translation and some work on the code and makefile. */
#include <libintl.h>	/* Mac OS X doesn't have libintl.h, so define "char *gettext();" then. */
#include <locale.h>
#endif

#if	READLINE
#if	0		/* The following two includes only needed if explicitly calling ncurses functions. */
#include <curses.h>
#include <term.h>
#endif
#include <readline/readline.h>
#include <readline/history.h>
#endif
#if	EDITLINE	/* Editline is a stripped down version of readline. */
#include <editline.h>
#endif

/* Include files from the current directory: */
#include "standard.h"	/* a standard include file for any math program written in C */
#include "am.h"		/* the main include file for Mathomatic, contains tunable parameters */
#include "complex.h"	/* floating point complex number arithmetic function prototypes */
#include "proto.h"	/* global function prototypes, made with cproto utility */
#include "altproto.h"	/* backup global function prototypes, in case of no proto.h */
#include "externs.h"	/* global variable extern definitions */
#include "blt.h"	/* blt() function definition */
