
                                  Mathomatic
                           Computer Algebra System

This file can be found in the Mathomatic source code directory, or at
http://mathomatic.org/README.txt for the most recent version.

This archive contains the complete C source code and documentation for
Mathomatic, the automatic algebraic manipulator. Mathomatic compiles and runs
under any operating system with a C compiler and is very portable. There are
no dependencies other than the standard C libraries. Using the GNU make
utility or shell scripts, this text mode application should compile with the
GNU C compiler (gcc), Tiny C compiler (tcc), or MinGW, and run correctly
under Linux, Unix, Mac OS X, Microsoft Windows, and many mobile devices,
without any modifications, except to the compile/install command-lines.

Most of the Mathomatic code can also be called from any C compatible program,
when linked with Mathomatic specially compiled as a symbolic math library
(see directory "lib").

Mathomatic is a free, portable, general-purpose Computer Algebra System (CAS)
and calculator software that can symbolically solve, simplify, combine, and
compare algebraic equations, simultaneously performing generalized standard,
complex number, modular, and polynomial arithmetic, as needed. It does some
calculus and is very easy to compile/install, learn, and use. Plotting
expressions with gnuplot is also supported.

New users: try typing "help examples". There are many interesting Mathomatic
scripts in the "tests" directory, that show what Mathomatic can do, and they
teach some mathematics, too.


                                   Licenses
                                   --------

All Mathomatic software and documentation in this archive are copyrighted.

The Mathomatic documentation in the "doc" directory is licensed under the GNU
Free Documentation License (GFDL) version 1.3, with no Invariant Sections, no
Front-Cover Texts, and no Back-Cover Texts, so it can be easily published,
corrected, and translated by anyone.

The Mathomatic software and all else are licensed under the GNU Lesser
General Public License (LGPL) version 2.1 (see file "COPYING" for the full
text of the license). That means Mathomatic is free software and comes with
no warranty at all, but if you find any errors in it, I will try to fix it.
LGPL also means that even closed-source, proprietary software can include and
make use of the Mathomatic symbolic math library, but please tell the chief
author (George Gesslein II) if you are using Mathomatic in your software.


                                  Compilation
                                  -----------

This section can be skipped; you may wish to jump to the next section with
short compile/install instructions or read the file "INSTALL.txt", if you
only wish to install Mathomatic.

On a Debian or Ubuntu Linux computer, the "build-essential" and readline
development packages "libreadline-dev" are required to compile Mathomatic
with readline functionality. To open a shell window in desktop Linux, launch
Applications/Accessories/Terminal. Makefiles are text files named "makefile",
read by GNU make, used to build and install software as requested and needed;
reading the comments contained within them is often helpful when using
makefiles.

The latest Mathomatic source code may be downloaded from:

	http://www.mathomatic.org/math/download.html

To extract and use the Mathomatic source code archive, make a new directory,
copy the archive to it and change directory to it, and extract the archive
with the unzip or tar utilities. unzip and tar extract to the current
directory, so be sure to change directory (cd) to where you want it, before
extracting. With "am.zip", be sure it is an empty directory, or you will end
up with a mix of many files and directories. All other archives on
Mathomatic.org properly put the files under a single directory created by the
archive, but "am.zip" puts many files in the current directory and a few
subdirectories.

To compile Mathomatic without readline support, type the name of the GNU make
utility (either "make" or "gmake", depending on the operating system) at the
shell prompt while in the Mathomatic source code directory. This will compile
the C source code using the instructions in file "makefile" to create the
executable file named "mathomatic". To run Mathomatic, type "./mathomatic" at
the shell prompt. No other files are needed to run Mathomatic, so the
executable can be copied to anywhere you like.

To test most functionality, type "make test" or "./t". If no errors are
encountered, "All tests passed 100% correctly." will be displayed, along with
the total run time. Otherwise the differences from the expected output will
be displayed.

To recompile with readline, which allows editing and history recall of all
Mathomatic line input by pressing the arrow keys, type:

	make clean
	make READLINE=1

That allows use of the cursor keys to recall and edit previously entered and
pushed expressions when running Mathomatic. A readline library must be
installed to compile for and use readline.

To create the compile-time secure version of Mathomatic, with readline
functionality and no file I/O nor shelling out possible, type:

	./compile.secure

That will create the executable "mathomatic_secure", which can safely be used
as a telnet application or CGI program. The run-time security option
"mathomatic -s4" functions the same as "mathomatic_secure", so this secure
version compilation is not ever required.


                                  Installation
                                  ------------

A typical compile/install is done by typing the following at the shell
prompt, while in the Mathomatic source code directory:

	make clean
	make READLINE=1
	make test
	sudo make m4install

That will compile the source code with readline enabled, test the
functionality of the generated executable (named "mathomatic"), and install
all executables, docs, and tests in "/usr/local" in less than a minute.

To clean, compile, test, and install the Mathomatic Prime Number Tools:

	cd primes
	make flush
	make
	./t
	sudo make install

To restore the Mathomatic distribution directory to pristine condition, type:

	make distclean

To uninstall all installed Mathomatic files from the local computer, type:

	sudo make uninstall

-----------------------------------------------------------------------------

There are quite a few math goodies in this archive, besides the main
Mathomatic program:

  The directory "doc" contains the Mathomatic User documentation in HTML.
  The directory "examples" contains example source code in C and Python.
  The directory "icons" contains Mathomatic icons for your desktop.
  The directory "lib" contains the API and test for the Mathomatic library.
  The directory "m4" contains the m4 (macro) Mathomatic supporting files.
  The directory "menu" contains the Debian Menu System files for Mathomatic.
  The directory "misc" contains a bug list and an idea list, etc.
  The directory "primes" contains a prime number generator utility, etc.
  The directory "tests" contains Mathomatic test and example scripts.

The "tests" directory is a good directory to visit to learn things about math
and Mathomatic.

For quick help while running Mathomatic, type "?" or use the help command. To
read or print the user documentation, point your web browser to the file
"doc/index.html", or "/usr/local/share/doc/mathomatic/html/index.html" if
Mathomatic was installed. When copying the Mathomatic documentation, please
copy the entire doc or html directory, not just selected HTML files from it.

For the most recent source code, binaries, documentation, information, and
other help, please visit the Mathomatic website: "http://www.mathomatic.org",
or its mirror site "http://mathomatic.orgserve.de/math/".

This README was written by George Gesslein II, chief author and maintainer of
Mathomatic:

  email:
    gesslein@mathomatic.org or
    georgegesslein@gmail.com

  postal address:
    George Gesslein II
    P.O. Box 224
    Lansing, New York  14882-0224
    USA

The author is happy to help people and other gentle beings with any problems
using this software. If you get stuck or find an error, send email to George
Gesslein II.

Please report suspected bugs in Mathomatic to the author via email, or on the
Launchpad website: "https://launchpad.net/mathomatic". Launchpad features a
complete bug management system. Mathomatic should always give correct,
simple, and beautiful answers; if not, please report it.

Many thanks to Jochen Plumeyer ("http://www.plumeyer.org") for donating
server space and services on the mirror site for a very long time. See the
"AUTHORS" file for people who have contributed code back to the Mathomatic
project.


             Compile-time defines for the Mathomatic source code
             ---------------------------------------------------

To compile Mathomatic for a desktop operating system like Unix, GNU/Linux,
Mac OS X, or any POSIX compliant OS, define UNIX on the C compiler command
line with "-DUNIX". To compile Mathomatic for a generic system, simply
compile with none of these defines. To compile for Microsoft Windows with the
MinGW gcc compiler, define MINGW for a completely stand-alone executable. To
compile for Microsoft Windows using CygWin, define CYGWIN; note that
compiling for CygWin will require the presence of CygWin to run the resulting
executable. To compile for embedded systems, or handhelds like the Nintendo
DS, or smartphones like the iPhone and Android, define HANDHELD for reduced
memory usage.

Define READLINE and include the readline libraries at link time to use
readline mode, allowing easy command-line editing and history recall by
pressing the arrow keys. The readline development package must be installed
to compile with this option and the readline library must be installed to run
the resulting executable.

Define DEBUG for more code correctness checking, more error checking, and
easier debugging. With DEBUG, entering Control-C will ask if you wish to
change the debug level or abort. Mathomatic will run slower with DEBUG
enabled. Never define DEBUG for a public release. This define has nothing to
do with the debug level ("set debug").

Define SILENT to *not* display most error messages, all helpful messages,
warnings, and debugging output code. This is useful when using Mathomatic in
another program or if you only want terse output. Code size is reduced with
this option and you will not be able to set a debug level. Setting the debug
level to -1 with the Mathomatic command "set debug -1" is another way to not
display helpful messages. "set debug -2" will not even display any warnings.

Define LIBRARY to compile the Mathomatic code as a symbolic math library, so
that the Mathomatic symbolic math engine may be used in any C compatible
program. SILENT=1 is automatically defined when LIBRARY is defined, because
the library usually works silently, with any display done by the developer's
code. To enable debugging and output of messages in the library, define
SILENT=0. See the directory "lib" for the library hooks (API) and
test/example program and how to compile it (there is an easy to use makefile
for this, read "lib/README.txt").

Define SECURE for no file I/O nor forking, to disable all file reading,
writing, and executing, except for I/O to standard input/output/error (the
default files in any C program, used for input, output, and displaying
errors). This is useful when making Mathomatic available to the public
through telnet or CGI programs. It is also useful when making ROMable or
stand-alone code. All insecure commands and code are omitted when SECURE is
defined. See file "compile.secure", which is the secure Mathomatic build
script. The run-time option -s4 does the same thing and makes this special
compilation for security unnecessary.

Define TIMEOUT_SECONDS to set the maximum number of seconds Mathomatic may
run. Upon timeout, Mathomatic properly exits. This is useful when making
Mathomatic a telnet or CGI program, so it won't overload or tie-up the
server.

Define HANDHELD when compiling for embedded systems, or handheld devices like
the iPhone or Android. Currently, this only reduces the maximum memory usage
by reducing maximum size of expressions. The default expression array size
will be reduced to be 6 times smaller to accommodate the smaller RAM size of
handhelds.

The I18N define is meant to enable internationalization using gettext(3).
Currently all strings to be translated have been marked with _(string), but
no translations have been made, so Mathomatic is only available in English.
gettext(3) translations may not be possible, because they may require the
Unicode character set and GNU automake, I think. Or a lot of work on the
makefile, and maybe the source code.

Define NO_COLOR to default to color mode off. Define BOLD_COLOR to default to
bold color mode. Otherwise the default is non-bold color mode on. The
Symbolic Math Library defaults to color mode off (NO_COLOR).

To see which of the above defines were used in a compiled version of
Mathomatic, use the "version status" command.


                         Mathomatic C source code files
                         ------------------------------

  altproto.h - alternate proto.h, function prototypes, made by hand
  am.h - the main include file for Mathomatic, contains tunable parameters
  blt.h - the fast memory copy routine, allows overlapping copies
  complex.h - floating point complex number arithmetic function prototypes
  externs.h - global variable extern definitions, from globals.c
  includes.h - automatically includes all necessary include files
  license.h - the current Mathomatic software license notice
  proto.h - all global function prototypes, made with cproto utility
  standard.h - a generally useful include file for C math programs

  am.c - standard routines for Mathomatic
  cmds.c - code for commands that don't belong anywhere else
  complex.c - floating point complex number routines for Mathomatic
  complex_lib.c - generic floating point complex number arithmetic library
  diff.c - symbolic differentiation routines and related commands
  factor.c - symbolic factorizing routines (not polynomial factoring)
  factor_int.c - floating point constant factorizing routines
  gcd.c - general floating point GCD and numerical fractions code
  globals.c - global variable and array definitions, duped in externs.h
  help.c - command table, help command, and input parsing routines
  integrate.c - integration routines and commands
  list.c - expression and equation display routines
  main.c - startup code for Mathomatic, not used for library
  parse.c - mathematical expression parsing routines
  poly.c - simplifying and polynomial routines
  simplify.c - simplifying routines
  solve.c - symbolic solving routines
  super.c - group and combine denominators of symbolic fractions
  unfactor.c - symbolic unfactorizing (expanding) routines


              Symbolic math library interface source code files
              -------------------------------------------------

  lib/mathomatic.h - include file for user programs

  lib/lib.c - commented Mathomatic symbolic math engine API
  lib/testmain.c - test and example program main() to link with library
