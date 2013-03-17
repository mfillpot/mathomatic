
                        Mathomatic Prime Number Tools
                        -----------------------------

This directory contains some small, command-line, integer math utilities
written in C and Python. To compile, test, and install these utilities, type
the following commands at the Unix shell prompt:

	make
	make test
	sudo make install

This will install:

	matho-primes - quickly generate consecutive prime numbers
	primorial - calculate large primorials
	matho-mult - multiply large integers
	matho-pascal - output Pascal's triangle
	matho-sum - sum large integers
	matho-sumsq - output the minimum sum of the squares of integers

These stand-alone utilities come with Mathomatic and they don't need to be
installed to work properly. See the website www.mathomatic.org to get the
latest version.

The Python program "primorial" is included in this directory for calculating
large primorials from matho-primes. A primorial is the product of all primes
up to the given number. To generate a list of all unique primorials from 2 to
97, type the following at the Unix shell after installing matho-primes:

	primorial `matho-primes 2 97`

Example that calculates the number of primes less than or equal to 1,000,000:

	matho-primes 0 1000000 | wc -w

The result should be 78498.
