/*
 * Calculate and display Pascal's triangle.
 *
 * Copyright (C) 2005 George Gesslein II.
 
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#if	!MINGW
#include <sys/ioctl.h>
#include <termios.h>
#endif

#define	true	1
#define	false	0

#ifndef	LDBL_DIG
#define	USE_DOUBLES	1
#warning "long doubles not supported by this C compiler."
#endif

#if	USE_DOUBLES
typedef double double_type;
#warning "Using only double floats instead of long double floats, as requested."
#else
typedef long double double_type;
#endif

#define	MAX_LINES	1000	/* max number of lines of Pascal's triangle allowed */

void	allocate_triangle(void);
void	calculate_triangle(void);
void	display_triangle(void);
int	center_buf(int line_no, int cell_size);
void	usage(int ev);

#if	USE_DOUBLES
int		precision = DBL_DIG;
#else
int		precision = LDBL_DIG;
#endif
int		glines = 26;
int		gcell_size = 6;
double_type	*garray[MAX_LINES];
int		screen_columns = 80;
int		centered = true;
char		gline_buf[1000];

char		*prog_name = "matho-pascal";

int
main(int argc, char *argv[])
{
#if	!MINGW
	struct winsize	ws;

	ws.ws_col = 0;
	ws.ws_row = 0;
	ioctl(1, TIOCGWINSZ, &ws);
	if (ws.ws_col > 0) {
		screen_columns = ws.ws_col;
	}
#endif
	if (screen_columns >= sizeof(gline_buf)) {
		screen_columns = sizeof(gline_buf) - 1;
	}

	switch (argc) {
	case 0:
	case 1:
		break;
	case 2:
		centered = false;
		if (isdigit(argv[1][0])) {
			glines = atoi(argv[1]);
			break;
		}
	default:
		usage(EXIT_FAILURE);
	}
	if (glines <= 0 || glines > MAX_LINES) {
		fprintf(stderr, "%s: Number of lines out of range (1..%d).\n", prog_name, MAX_LINES);
		exit(EXIT_FAILURE);
	}
	allocate_triangle();
	calculate_triangle();
	display_triangle();
	exit(EXIT_SUCCESS);
}

void
allocate_triangle(void)
{
	int	i;

	for (i = 0; i < glines; i++) {
		garray[i] = (double_type *) calloc(i + 1, sizeof(double_type));
		if (garray[i] == NULL) {
			fprintf(stderr, "%s: Not enough memory.\n", prog_name);
			exit(EXIT_FAILURE);
		}
	}
}

void
calculate_triangle(void)
{
	int	i, j;

	for (i = 0; i < glines; i++) {
		for (j = 0; j <= i; j++) {
			if (j == 0 || j == i) {
				garray[i][j] = 1.0;	/* the line begins and ends with 1 */
			} else {
				garray[i][j] = garray[i-1][j-1] + garray[i-1][j];
			}
		}
	}
}

void
display_triangle(void)
{
	int		i, j;
	int		len;

	if (centered && glines > 20) {
		len = center_buf(19, 8);
		if (len > 0 && len < screen_columns) {
			gcell_size = 8;	/* for very wide screens */
		}
	}
	for (i = 0; i < glines; i++) {
		if (centered) {
			len = center_buf(i, gcell_size);
			if (len <= 0 || len >= screen_columns) {
				return;	/* stop here because of wrap-around */
			}
			/* center on screen */
			for (j = (screen_columns - len) / 2; j > 0; j--) {
				printf(" ");
			}
			printf("%s", gline_buf);
		} else {
			for (j = 0; j <= i; j++) {
#if	USE_DOUBLES
				printf("%.*g ", precision, garray[i][j]);
#else
				printf("%.*Lg ", precision, garray[i][j]);
#endif
			}
		}
		printf("\n");
	}
}

/*
 * Create a line of output in gline_buf[] for centering mode.

 * Return length if successful, otherwise return 0.
 */
int
center_buf(int line_no, int cell_size)
{
	int	j, k;
	int	i1;
	int	len;
	char	buf2[100];

	assert(line_no < glines);
	gline_buf[0] = '\0';
	for (j = 0; j <= line_no; j++) {
#if	USE_DOUBLES
		len = snprintf(buf2, sizeof(buf2), "%.*g", precision, garray[line_no][j]);
#else
		len = snprintf(buf2, sizeof(buf2), "%.*Lg", precision, garray[line_no][j]);
#endif
		assert(len > 0);
		if (len >= cell_size) {
			return(0);	/* cell_size too small */
		}
		/* center in the cell */
		for (k = i1 = (cell_size - len) / 2; k > 0; k--) {
			strcat(gline_buf, " ");
		}
		strcat(gline_buf, buf2);
		for (k = len + i1; k < cell_size; k++) {
			strcat(gline_buf, " ");
		}
	}
	return(strlen(gline_buf));
}

void
usage(int ev)
{
	printf("Usage: %s [number-of-lines]\n\n", prog_name);
	printf("Display up to %d lines of Pascal's triangle.\n", MAX_LINES);
	printf("If number-of-lines is specified, don't center output.\n");
	printf("Number of digits of precision is %d.\n", precision);
	exit(ev);
}
