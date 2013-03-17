
 examples/README.txt
 -------------------

See directory ../tests for example, tutorial, and test scripts for
Mathomatic.

This directory is where example source code goes for a binary distribution.

-----------------------------------------------------------------------------

limits.c - C program to display current C data type limits and sizes.
           Mathomatic is limited to double precision floating point.
           Use "./compile.limits" to compile.

roots.c - Nice GSL example of a numerical polynomial equation solver utility.
          Compile with "./c", requires the libgsl development files.
          Like Mathomatic, uses complex number double precision output.
          Use "./compile.roots" to compile.

testprimes - A parallel, brute force test of the prime number generator.
             Checks the first 50,000,000 primes for gaps or errors.
             Requires matho-primes and BSD Games primes to be installed.
             It simply compares their output up to 1,000,000,000.

-----------------------------------------------------------------------------

This directory also contains factorial functions "factorial()" in various
computer languages are for use with output from the Mathomatic code command,
which converts factorial expressions like x! to factorial(x).

Type "./factorial" followed by integers or integer expressions to compute
large factorials with Python and test "fact.py".

-----------------------------------------------------------------------------

These files do not have any man page, because I think they are not ready or
are not made for packaging. They are just example programs. Any requests to
make man pages for these files will be honored by the author of this
document: George Gesslein II. Please specify which utilities you think should
be installable with a man page.
