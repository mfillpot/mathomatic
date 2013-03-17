/*
 * Generate batches of consecutive prime numbers using a modified Sieve of Eratosthenes
 * algorithm that doesn't use much memory by using a windowing sieve buffer.
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
 * Changes:
 *
 * 11/22/05 - converted everything to long doubles.  Now uses C99 long double functions.
 * 3/25/06 - made primes buffer variable size.
 * 3/30/08 - Allow long double to be aliased to double when long double isn't supported.
 * 2/11/09 - Cleanup calculation of number of decimal digits and max_integer.
 * 9/12/10 - General cleanup and added error message for when the requested number of primes to display is not reached.
 * 10/25/10 - Added -c and -h options.
 * 10/26/10 - Using usage2() instead of usage() most of the time, now.
 * 10/28/10 - Fixed to work with 16-bit integers.
 * 1/16/11 - Allow to use doubles instead of long doubles with USE_DOUBLES define, for systems that don't fully support long doubles.
 * 5/10/11 - Added -u (unbuffered output) option, for real-time output.
 * 11/13/11 - Use EXIT_SUCCESS and EXIT_FAILURE macros.
 * 11/30/11 - Added -m option.
 * 12/31/11 - Added a compile-time warning when the number of digits of precision of long doubles is less than 18.
 * 07/11/12 - Added -v option to display program name with version number and then exit successfully.
 * 08/09/12 - Allow "matho-primes all" for endless output of consecutive primes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#if	!NO_GETOPT_H
#include <getopt.h>
#endif

#define	VERSION		"1.4"	/* The current version number of this primes program. */

#define	true	1
#define	false	0

#ifndef	max
#define max(a, b)    (((a) > (b)) ? (a) : (b))		/* return the maximum of two values */
#endif

#ifndef	min
#define min(a, b)    (((a) < (b)) ? (a) : (b))		/* return the minimum of two values */
#endif

#define	ARR_CNT(a)	((int) (sizeof(a)/sizeof(a[0])))	/* returns the number of elements in an array */

#if	!USE_DOUBLES
#ifndef	LDBL_DIG
#define	USE_DOUBLES	1
#warning "long doubles not supported by this C compiler."
#elif LDBL_DIG < 18
#warning "long double number of digits precision is less than 18, probably meaning matho-primes is double precision only."
#warning "Due to the above floating point warning, more testing or compiling with -DUSE_DOUBLES is recommended."
#endif
#endif

#if	USE_DOUBLES
typedef double double_type;
#warning "Using only double floats instead of long double floats, as requested."
#else
typedef long double double_type;
#endif

/* Maximum memory usage in bytes; can be set to any size. */
#ifndef	BUFFER_SIZE
#define BUFFER_SIZE	2000000
#endif
#if	BUFFER_SIZE >= (INT_MAX / 2) || BUFFER_SIZE < 100
#warning BUFFER_SIZE out of range, using default.
#undef BUFFER_SIZE
#define BUFFER_SIZE	min((INT_MAX / 2), 2000000)
#endif

void		generate_primes(void);
int		test_pal(double_type d, double_type base);
void		usage(int ev);
void		usage2(int ev);
int		get_double_type_int(char *cp, double_type *dp);
#if	!USE_DOUBLES
long double	powl(), ceill(), sqrtl(), fmodl(), strtold();
#endif

double_type max_integer;		/* largest value of a double_type integral value */
double_type start_value;		/* where to start finding primes */
double_type number;			/* number of prime lines to display */
int count_requested;			/* true if the number of primes to display is set by "number" above */
double_type default_number = 20;	/* default number of primes to display */
double_type end_value;			/* where to stop finding primes */
double_type skip_multiples[] = {	/* Additive array that skips over multiples of 2, 3, 5, and 7. */
	10, 2, 4, 2, 4, 6, 2, 6,
	 4, 2, 4, 6, 6, 2, 6, 4,
	 2, 6, 4, 6, 8, 4, 2, 4,
	 2, 4, 8, 6, 4, 6, 2, 4,
	 6, 2, 6, 6, 4, 2, 4, 6,
	 2, 6, 4, 2, 4, 2,10, 2
};	/* sum of all numbers = 210 = (2*3*5*7) */

int		pal_flag, twin_flag;
double_type	pal_base = 10;	/* The palindrome base, if displaying palindromic primes. */

char		*prime;		/* The boolean sieve array (buffer) for the current window of numbers being tested for primality; */
				/* each char (byte) contains true or false, true if prime. */
int		buffer_size;	/* Number of bytes for variable size prime[] buffer above. */

char		*prog_name = "matho-primes";

int
main(int argc, char *argv[])
{
#if	NO_GETOPT_H	/* if no getopt.h is available */
	extern char	*optarg;	/* set by getopt(3) */
	extern int	optind;
#endif

	int		i;
	char		buf[1000];
	char		*cp = NULL;
	double		new_size = 0;

	buffer_size = BUFFER_SIZE;
/* set the highest number this program will work with: */
#if	USE_DOUBLES
	max_integer = pow(10.0, (double) (DBL_DIG));
#else
	max_integer = powl(10.0L, (long double) (LDBL_DIG));
#endif
	while (max_integer == max_integer + 1.0) {
#if	USE_DOUBLES
		fprintf(stderr, "Warning: max_integer (%g) is too large; size of double = %u bytes.\n", max_integer, (unsigned) sizeof(double));
		max_integer /= 10.0;
#else
		fprintf(stderr, "Warning: max_integer (%Lg) is too large; size of long double = %u bytes.\n", max_integer, (unsigned) sizeof(long double));
		max_integer /= 10.0L;
#endif
	}
#if	MINGW || __APPLE__
do_again:
#endif
	start_value = -1.0;
	end_value = max_integer;
	number = 0;
	count_requested = false;
/* process command line options: */
	while ((i = getopt(argc, argv, "c:thuvp:m:")) >= 0) {
		switch (i) {
		case 'c':
			count_requested = true;
			if (optarg && !get_double_type_int(optarg, &number)) {
				usage2(EXIT_FAILURE);
			}
			break;
		case 'h':
			usage2(EXIT_SUCCESS);
			break;
		case 't':
			twin_flag = true;
			break;
		case 'p':
			pal_flag = true;
			if (optarg && !get_double_type_int(optarg, &pal_base)) {
				usage2(EXIT_FAILURE);
			}
			break;
		case 'm':
			if (optarg)
				new_size = strtod(optarg, &cp) * BUFFER_SIZE;
			if (optarg == NULL || cp == NULL || *cp || new_size < 100 || new_size >= (INT_MAX / 2)) {
				fprintf(stderr, "%s: Invalid memory size multiplier specified.\n", prog_name);
				exit(EXIT_FAILURE);
			}
			buffer_size = (int) new_size;
			fprintf(stderr, "%s: Window size = %d bytes.\n", prog_name, buffer_size);
			break;
		case 'u':
			setbuf(stdout, NULL);	/* make output unbuffered */
			setbuf(stderr, NULL);
			break;
		case 'v':
			printf("%s version %s\n", prog_name, VERSION);
			exit(0);
		default:
			usage2(EXIT_FAILURE);
		}
	}
	for (; argc > optind; optind++) {
		if (strcasecmp(argv[optind], "all") == 0) {
			if (start_value < 0)
				start_value = 0;
			count_requested = true;
			number = max_integer;
			continue;
		}
		if (strcasecmp(argv[optind], "twin") == 0) {
			twin_flag = true;
			continue;
		}
		break;
	}
	if (argc > optind && isdigit(argv[optind][0])) {
		if (get_double_type_int(argv[optind], &start_value)) {
			optind++;
		} else {
			usage2(EXIT_FAILURE);
		}
		if (argc > optind && isdigit(argv[optind][0])) {
			if (get_double_type_int(argv[optind], &end_value)) {
				if (end_value < start_value) {
					fprintf(stderr, "End value is less than start value.\n");
					usage2(EXIT_FAILURE);
				}
				optind++;
				if (number == 0)
					number = max_integer;
			} else {
				usage2(EXIT_FAILURE);
			}
		}
	}
	for (; argc > optind; optind++) {
		if (strcasecmp(argv[optind], "all") == 0) {
			if (start_value < 0)
				start_value = 0;
			count_requested = true;
			number = max_integer;
			continue;
		}
		if (strcasecmp(argv[optind], "twin") == 0) {
			twin_flag = true;
			continue;
		}
		break;
	}
	if (argc > optind) {
		if (strncasecmp(argv[optind], "pal", 3) == 0) {
			pal_flag = true;
			optind++;
		} else {
			fprintf(stderr, "Unrecognized argument: \"%s\".\n", argv[optind]);
			usage(EXIT_FAILURE);
		}
		if (argc > optind && isdigit(argv[optind][0])) {
			if (!get_double_type_int(argv[optind], &pal_base)) {
				usage(EXIT_FAILURE);
			}
			optind++;
		}
	}
	for (; argc > optind; optind++) {
		if (strcasecmp(argv[optind], "all") == 0) {
			if (start_value < 0)
				start_value = 0;
			count_requested = true;
			number = max_integer;
			continue;
		}
		if (strcasecmp(argv[optind], "twin") == 0) {
			twin_flag = true;
			continue;
		}
		break;
	}
	if (argc > optind) {
		fprintf(stderr, "Unrecognized argument: \"%s\".\n", argv[optind]);
		usage(EXIT_FAILURE);
	}
	if (pal_base < 2 || pal_base >= INT_MAX) {
		fprintf(stderr, "Palindrome number base must be >= 2.\n");
		usage(EXIT_FAILURE);
	}
	if (start_value < 0.0) {
get1:
		fprintf(stderr, "Enter number to start finding consecutive primes at (0): ");
		fflush(NULL);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			exit(EXIT_SUCCESS);
		switch (buf[0]) {
		case '\0':
		case '\n':
		case '\r':
			start_value = 0;
			break;
		default:
			if (!get_double_type_int(buf, &start_value)) {
				goto get1;
			}
		}
	}
	if (number == 0) {
get2:
		fprintf(stderr, "Enter number of%s%s primes to output (0 to end)",
		    pal_flag ? " palindromic" : " consecutive", twin_flag ? " twin" : "");
#if	USE_DOUBLES
		fprintf(stderr, " (%g): ", default_number);
#else
		fprintf(stderr, " (%Lg): ", default_number);
#endif
		fflush(NULL);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			exit(EXIT_SUCCESS);
		switch (buf[0]) {
		case '\0':
		case '\n':
		case '\r':
			number = default_number;
			break;
		default:
			if (!get_double_type_int(buf, &number)) {
				goto get2;
			}
		}
		count_requested = true;
	}
	if (prime == NULL) {
/* allocate the prime[] buffer: */
		prime = (char *) malloc(buffer_size);
	}
	if (prime == NULL) {
		fprintf(stderr, "%s: Not enough memory for buffer_size = %d.\n", prog_name, buffer_size);
		exit(EXIT_FAILURE);
	}
	generate_primes();
#if	MINGW || __APPLE__
	fflush(NULL);
	if (argc <= 1 && number > 0) {
		goto do_again;
	}
#endif
	exit(EXIT_SUCCESS);
}

/*
 * Eliminate all multiples of "arg" from the sieve array by setting their entries to false.
 * The sieve array "prime[]" is the prime truth values of a consecutive
 * batch of numbers starting at "start_value".
 *
 * When all multiplies of all primes up to the square root
 * of the highest value represented in sieve array are registered,
 * the sieve array will tell whether the numbers in it are prime or not.
 * This is how the Sieve of Eratosthenes works.
 */
void
elim_factor(double_type arg)
{
	double_type	d;
	int		i, j;

#if	USE_DOUBLES
	d = ceil(start_value / arg);
#else
	d = ceill(start_value / arg);
#endif
	if (d < 2.0)
		d = 2.0;
	d *= arg;
	d -= start_value;
	if (d >= buffer_size)
		return;
	i = (int) d;
	if (i >= buffer_size)
		return; 
	assert(i >= 0);
	if (arg >= buffer_size) {
		prime[i] = false;
	} else {
		j = (int) arg;
		assert(j > 0);
		for (; i < buffer_size; i += j) {
			prime[i] = false;
		}
	}
}

/*
 * Generate and display at most "number" consecutive prime numbers,
 * between "start_value" and "end_value".
 */
void
generate_primes(void)
{
	int		n, j;
	double_type	count, d, ii, sqrt_value, last_prime = -3.0;

	for (count = 0; count < number; start_value += buffer_size) {
		if (start_value > end_value) {
			goto check_return;
		}
		/* generate a batch of consecutive primes with the prime number sieve */
		memset(prime, true, buffer_size);	/* set the prime array to all true */
#if	USE_DOUBLES
		sqrt_value = 1.0 + sqrt(min(start_value + (double) buffer_size, end_value));
#else
		sqrt_value = 1.0 + sqrtl(min(start_value + (long double) buffer_size, end_value));
#endif
		elim_factor((double_type) 2.0);
		elim_factor((double_type) 3.0);
		elim_factor((double_type) 5.0);
		elim_factor((double_type) 7.0);
		d = 1.0;
		while (d <= sqrt_value) {
			for (j = 0; j < ARR_CNT(skip_multiples); j++) {
				d += skip_multiples[j];
				elim_factor(d);
			}
		}
		/* display the requested part of the batch of generated prime numbers */
		for (n = 0; n < buffer_size && count < number; n++) {
			if (prime[n]) {	/* if prime number */
				ii = start_value + n;
				if (ii > end_value) {
					goto check_return;
				}
				if (ii > 1.0) {
					if (pal_flag && !test_pal(ii, pal_base))
						continue;
					if (twin_flag) {
						if ((last_prime + 2.0) == ii) {
#if	USE_DOUBLES
							printf("%.0f %.0f\n", last_prime, ii);
#else
							printf("%.0Lf %.0Lf\n", last_prime, ii);
#endif
							count++;
						}
					} else {
#if	USE_DOUBLES
						printf("%.0f\n", ii);
#else
						printf("%.0Lf\n", ii);
#endif
						count++;
					}
					last_prime = ii;
				}
			}
		}
	}
check_return:
	if (count_requested && count < number) {
		fprintf(stderr, "%s: Number of primes requested not reached.\n", prog_name);
		exit(EXIT_FAILURE);
	}
}

/*
 * Parse a space or null terminated ASCII number in the string pointed to by cp.
 *
 * Return true with a floating point double_type value in *dp
 * if a valid positive integer or zero,
 * otherwise display an error message and return false.
 */
int
get_double_type_int(char *cp, double_type *dp)
{
	char	*cp1;

	if (cp == NULL || dp == NULL) {
		return false;
	}
	errno = 0;
#if	USE_DOUBLES
	*dp = strtod(cp, &cp1);
#else
	*dp = strtold(cp, &cp1);
#endif
	if (errno) {
		perror(NULL);
		return false;
	}
	if (cp == cp1) {
		fprintf(stderr, "Invalid number.\n");
		return false;
	}
	switch (*cp1) {
	case '\0':
	case '\r':
	case '\n':
		break;
	default:
		if (isspace(*cp1)) {
			break;
		}
		fprintf(stderr, "Invalid number.\n");
		return false;
	}
	if (*dp > max_integer) {
#if	USE_DOUBLES
		fprintf(stderr, "Number is too large, maximum is %g.\n", max_integer);
#else
		fprintf(stderr, "Number is too large, maximum is %Lg.\n", max_integer);
#endif
		return false;
	}
#if	USE_DOUBLES
	if (*dp < 0.0 || fmod(*dp, 1.0) != 0.0) {
#else
	if (*dp < 0.0 || fmodl(*dp, 1.0L) != 0.0) {
#endif
		fprintf(stderr, "Number must be a positive integer or zero.\n");
		return false;
	}
	return true;
}

/*
 * Return true if "d" is a palindromic number, base "base".
 */
int
test_pal(double_type d, double_type base)
{
#define	MAX_DIGITS	1000

	int		i, j;
	int		digits[MAX_DIGITS];

	/* build the array of digits[] */
	for (i = 0; d >= 1.0; i++) {
		assert(i < MAX_DIGITS);
#if	USE_DOUBLES
		digits[i] = (int) (fmod(d, base));
#else
		digits[i] = (int) (fmodl(d, base));
#endif
		d /= base;
	}
	/* compare the array of digits[] end to end */
	for (j = 0, i--; j < i; j++, i--) {
		if (digits[i] != digits[j])
			return false;
	}
	return true;
}

void
usage(int ev)
{
	printf("Prime number generator version %s\n", VERSION);
	printf("Usage: %s [start [stop] or \"all\"] [\"twin\"] [\"pal\" [base]]\n\n", prog_name);
#if	USE_DOUBLES
	printf("Generate consecutive prime numbers from start to stop, up to %g.\n", max_integer);
#else
	printf("Generate consecutive prime numbers from start to stop, up to %Lg.\n", max_integer);
#endif
	printf("If \"twin\" is specified, output only twin primes.\n");
	printf("If \"pal\" is specified, output only palindromic primes.\n");
	printf("The palindrome number base may be specified, the default is base 10.\n");
	exit(ev);
}

void
usage2(int ev)
{
	printf("Prime number generator version %s\n", VERSION);
	printf("Usage: %s [options] [start [stop]]\n\n", prog_name);
#if	USE_DOUBLES
	printf("Generate consecutive prime numbers from start to stop, up to %g.\n", max_integer);
#else
	printf("Generate consecutive prime numbers from start to stop, up to %Lg.\n", max_integer);
#endif
	printf("Options:\n");
	printf("  -c count         Count lines of primes, stop when count reached.\n");
	printf("  -h               Display this help and exit.\n");
        printf("  -m number        Specify a memory size multiplier.\n");
	printf("  -p base          Output only palindromic primes.\n");
	printf("  -t               Output only twin primes.\n");
	printf("  -u               Set all output to be unbuffered.\n");
	printf("  -v               Display version number, then exit successfully.\n");
	exit(ev);
}
