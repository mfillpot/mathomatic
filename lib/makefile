# Makefile for the Mathomatic symbolic math library and its test program.
# See file README.txt for instructions.

SHELL		= /bin/sh # from http://www.gnu.org/prep/standards/
CC		?= gcc # C compiler to use
INSTALL		?= install # installer to use
INSTALL_PROGRAM	?= $(INSTALL) # command to install executable program files
INSTALL_DATA	?= $(INSTALL) -m 0644 # command to install data files

VERSION		= `cat ../VERSION`
OPTFLAGS	?= -g -O3 -Wall -Wshadow -Wno-char-subscripts -Wno-unused-variable # gcc specific flags; can be removed
CFLAGS		?= $(OPTFLAGS)
CFLAGS		+= -fexceptions -DLIBRARY -DVERSION=\"$(VERSION)\" # necessary C compiler flags
LDLIBS		+= -lm # system libraries to link

# Install directories follow; installs everything in $(DESTDIR)/usr/local by default.
prefix		?= /usr/local
mandir		?= $(prefix)/share/man
libdir		?= $(prefix)/lib
includedir	?= $(prefix)/include

AOUT		= testmain # The name of the library test executable file to create.
LIB		= libmathomatic.a # The name of the symbolic math library file to create.
HEADERS		= mathomatic.h

MATHOMATIC_OBJECTS += globals.o am.o solve.o help.o parse.o cmds.o simplify.o \
		  factor.o super.o unfactor.o poly.o diff.o integrate.o \
		  complex.o complex_lib.o list.o gcd.o factor_int.o

# man pages to automatically make and install:
MAN3		= matho_init.3 matho_clear.3 matho_parse.3 matho_process.3

.PHONY: all install uninstall clean distclean maintainer-clean flush lib manpages

all: lib $(AOUT)

lib: $(LIB) $(MAN3)

$(LIB): lib.o $(MATHOMATIC_OBJECTS)
	$(AR) cr $(LIB) $+
	-ranlib $(LIB)
	@echo
	@echo Symbolic math library $(LIB) created.
	@echo

lib.o $(MATHOMATIC_OBJECTS): $(HEADERS) ../includes.h ../license.h ../standard.h ../am.h ../externs.h ../blt.h ../complex.h ../proto.h ../altproto.h ../VERSION

$(MATHOMATIC_OBJECTS): %.o: ../%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(AOUT): testmain.o $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $+ $(LDLIBS) -o $(AOUT)
	@echo
	@echo ./$(AOUT) created.

example: example.o $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $+ $(LDLIBS) -o example
	@echo
	@echo ./example created.

# Generate the library man pages, if not already made.
# Requires the very latest version of txt2man.
manpages $(MAN3): lib.c
	src2man -r "Mathomatic" -v "Symbolic Math Library" $+

install:
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man3
	$(INSTALL_DATA) $(LIB) $(DESTDIR)$(libdir)
	$(INSTALL_DATA) $(HEADERS) $(DESTDIR)$(includedir)
	$(INSTALL_DATA) $(MAN3) $(DESTDIR)$(mandir)/man3
	@echo
	@echo Mathomatic Symbolic Math Library installed.

uninstall:
	cd $(DESTDIR)$(mandir)/man3 && rm -f $(MAN3)
	cd $(DESTDIR)$(includedir) && rm -f $(HEADERS)
	rm -f $(DESTDIR)$(libdir)/$(LIB)
	@echo
	@echo Symbolic Math Library uninstall completed.

clean:
	rm -f *.o

distclean flush: clean
	rm -f $(AOUT) example
	rm -f *.a
	rm -f *.exe

maintainer-clean: distclean
	rm -f $(MAN3)
