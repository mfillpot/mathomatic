/*
 * Find and display the minimum sum of the squares for integers.
 *
 * Copyright (C) 2007 George Gesslein II.
 
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
 * Usage: matho-sumsq [numbers]
 *
 * This program finds the minimum number of positive integers squared
 * and summed to equal a given positive integer.  If the number of squared
 * integers is 2 (default "multi"), all combinations with 2 squares are displayed,
 * otherwise only the first solution found is displayed.
 *
 * If nothing is specified on the command line, the program gets its numbers from standard input.
 *
 * This file is mostly useful for testing the long integer square root function lsqrt() in file "lsqrt.c".
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#define	true	1
#define	false	0

#define	squared(a)	((a) * (a))

int	multi = 2;	/* display all combinations for this number of squares or less */

long	squares[4];

long	lsqrt(long y);

/*
 * Find the sum of "n" squares that equal "d1" and display if found.
 *
 * Return false if not found.
 */
int
sumsq(long d1, int n)
{
	int	i = 0;
	long	d2;
	long	save = 0;
	int	found_one = false;

	d2 = d1;
try_again:
	for (;;) {
		if (i == 2) {
			save = d2;
		}
		squares[i] = lsqrt(d2);
		assert(squares[i] >= 0);
		d2 -= squared(squares[i]);
		i++;
		if (i >= n) {
			break;
		}
	}
	if (d2 == 0) {
		for (i = 0; i < n; i++) {
			d2 += squared(squares[i]);
		}
		if (d2 != d1) {
			fprintf(stderr, "Error: Result doesn't compare identical to original number!\n");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < (n - 1); i++) {
			if (squares[i] < squares[i+1]) {
				goto skip_print;
			}
		}
		found_one = true;
		printf("%ld = %ld^2", d1, squares[0]);
		for (i = 1; i < n; i++) {
			if (squares[i] != 0)
				printf(" + %ld^2", squares[i]);
		}
		printf("\n");
skip_print:
		assert(found_one);
		if (n < 2 || n > multi) {
			return found_one;
		}
	}
	switch (n) {
	case 4:
		if (squares[2] > squares[n-1]) {
			squares[2] -= 1;
			d2 = save - squared(squares[2]);
			i = 3;
			goto try_again;
		}
	case 3:
		if (squares[1] > squares[n-1]) {
			squares[1] -= 1;
			d2 = d1 - squared(squares[0]) - squared(squares[1]);
			i = 2;
			goto try_again;
		}
	case 2:
		if (squares[0] > squares[n-1]) {
			squares[0] -= 1;
			d2 = d1 - squared(squares[0]);
			i = 1;
			goto try_again;
		}
	}
	return found_one;
}

/*
 * Display the shortest sums of squares for long integer "d1".
 *
 * Return the minimum number of summed squares required to represent "d1".
 */
int
findsq(long d1)
{
	if (sumsq(d1, 1))
		return 1;
	if (sumsq(d1, 2))
		return 2;
	if (sumsq(d1, 3))
		return 3;
	if (sumsq(d1, 4))
		return 4;
	fprintf(stderr, "Whoops!  Can't find the sum of four squares that equal %ld.\n", d1);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int	i;
	long	d1 = 0;
	char	*cp, buf[1000];

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			errno = 0;
			d1 = strtol(argv[i], &cp, 10);
			if (errno) {
				perror(argv[i]);
				exit(EXIT_FAILURE);
			}
			if (d1 < 0) {
				fprintf(stderr, "Invalid command-line argument: \"%s\", positive integer required.\n", argv[i]);
				exit(EXIT_FAILURE);
			}
			if (*cp == '+') {
				for (;; d1 += 1) {
					findsq(d1);
				}
				exit(EXIT_SUCCESS);
			}
			if (argv[i][0] == '\0' || *cp) {
				fprintf(stderr, "Invalid number: \"%s\".\n", argv[i]);
				exit(EXIT_FAILURE);
			}
			findsq(d1);
		}
	} else {
		for (fflush(NULL); fgets(buf, sizeof(buf), stdin); fflush(NULL)) {
			errno = 0;
			d1 = strtol(buf, &cp, 10);
			if (errno) {
				perror(NULL);
				continue;
			}
			if (cp != buf && d1 == 0) {
				exit(EXIT_SUCCESS);
			}
			if (d1 <= 0) {
				fprintf(stderr, "Positive integer required; 0 to quit.\n");
				continue;
			}
			findsq(d1);
		}
	}
	exit(EXIT_SUCCESS);
}
