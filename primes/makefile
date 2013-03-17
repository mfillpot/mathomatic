# Makefile for compiling and installing the Mathomatic Prime Number Tools
# under a Unix-like system.
# See file README.txt for instructions.
#
# If make fails due to incomplete "long double" support, try:
#	CFLAGS="-O3 -DUSE_DOUBLES" make


SHELL		= /bin/sh # from http://www.gnu.org/prep/standards/
CC		?= gcc # C compiler to use
INSTALL		?= install # installer to use
INSTALL_PROGRAM	?= $(INSTALL) # Command to install executable program files.
INSTALL_DATA	?= $(INSTALL) -m 0644 # Command to install data files.

CC_OPTIMIZE	= -O3 # Default C compiler optimization flags that are safe.
#CC_OPTIMIZE	+= -ffast-math -fomit-frame-pointer # Extra optimize for speed, not functional under Mac OS X.

OPTFLAGS	?= $(CC_OPTIMIZE) -Wall -Wshadow -Wno-char-subscripts # gcc specific flags; change for other C compilers.
CFLAGS		?= $(OPTFLAGS)
#CFLAGS		+= -std=gnu99 # Uses the Gnu99 standard; may require setting this on gcc command line if using an older compiler.
LDLIBS		+= -lm

# Install directories follow, installs everything in $(DESTDIR)/usr/local by default.
prefix		?= /usr/local
bindir		?= $(prefix)/bin
mandir		?= $(prefix)/share/man

# The executables to create:
TARGETS		= matho-primes matho-pascal matho-sumsq
# The Python scripts to install:
PYTHON_SCRIPTS	= primorial matho-mult matho-sum
# The executables to install:
EXECUTABLES	= $(TARGETS) $(PYTHON_SCRIPTS)
# The man pages to install:
MAN1		= matho-primes.1 matho-pascal.1 matho-sumsq.1 primorial.1 matho-mult.1 matho-sum.1

all static: $(TARGETS) # make these by default
	@echo
	@echo Prime Number Tools successfully created.

# Strip the binaries of their symbol tables.  Makes smaller executables, but debugging becomes impossible.
strip: $(TARGETS)
	strip $(TARGETS)

matho-sumsq: matho-sumsq.o lsqrt.o
	$(CC) $(CFLAGS) $(LDFLAGS) $+ $(LDLIBS) -o $@

# To compile the Mathomatic Prime Number Tools as stand-alone executables
# that have no shared library dependencies, type "make flush static".
static: LDFLAGS += -static

test:
	@echo Testing basic functionality of matho-primes:
	time -p ./matho-primes 10000000000000 10000000300000 twin >test.out && diff -u --strip-trailing-cr twins.out test.out
	@rm test.out
	@echo Small primes test passed.
	@echo

# Same as "make test", except doesn't run the time command.
check:
	@echo Testing basic functionality of matho-primes:
	./matho-primes 10000000000000 10000000300000 twin >test.out && diff -u --strip-trailing-cr twins.out test.out
	@rm test.out
	@echo Small primes test passed.
	@echo

bigtest:
	@echo Testing that long doubles are fully functional in matho-primes:
	time -p ./matho-primes 100000000000000000 100000000000300000 twin >bigtest.out && diff -u --strip-trailing-cr bigtwins.out bigtest.out
	@rm bigtest.out
	@echo Big primes test passed.
	@echo

# Same as "make bigtest", except doesn't run the time command.
bigcheck:
	@echo Testing that long doubles are fully functional in matho-primes:
	./matho-primes 100000000000000000 100000000000300000 twin >bigtest.out && diff -u --strip-trailing-cr bigtwins.out bigtest.out
	@rm bigtest.out
	@echo Big primes test passed.
	@echo

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1
	$(INSTALL_PROGRAM) $(EXECUTABLES) $(DESTDIR)$(bindir)
	$(INSTALL_DATA) $(MAN1) $(DESTDIR)$(mandir)/man1
	@echo
	@echo The Prime Number Tools are installed.

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(EXECUTABLES)
	cd $(DESTDIR)$(mandir)/man1 && rm -f $(MAN1)
	@echo
	@echo Prime Number Tools uninstall completed.

clean:
	rm -f *.o *.pyc *.pyo
	rm -f test.out bigtest.out

flush distclean maintainer-clean: clean
	rm -f *.a
	rm -f $(TARGETS)
	rm -f *.exe
