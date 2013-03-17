
                                m4 Mathomatic
                                -------------

The executable named "mathomatic" is the best to run, however if you would
like functions, you need to run "rmath", which runs Mathomatic using GNU m4
as a macro pre-processor, allowing easy entry of standard math functions like
sqrt(x) and sin(x) (no logarithm function yet). "rmath" and "matho" are shell
scripts and are only known to work with GNU software. "rmath" runs "matho"
with a readline wrapper (rlwrap), if available. "matho" runs m4, reading
"functions.m4", and piping the output into Mathomatic. m4 Mathomatic will not
run under MS-Windows, unless m4 is provided by the third-party system CygWin.

To permanently install these program files along with their man pages on a
Unix-like system, if this is a binary distribution, type:

	sudo ./matho-install

for trig functions with radian units. For trig with degree units, type:

	sudo ./matho-install-degrees

These install commands may be repeated, the last one entered is current. This
binary distribution archive must be extracted to a directory and shell must
be running for the installation to work.

To undo the above commands and uninstall Mathomatic from your system, type:

	sudo ./matho-uninstall

Installation is *not* necessary to run Mathomatic.

"sudo make m4install" or "sudo make m4install-degrees" is the proper way to
install these files from the source code distribution.

Defined m4 macros for functions and constants are listed in the file
"functions.m4" and the rmath man page. All trigonometric functions (sin(x),
tan(x), etc.) are implemented as complex exponentials in m4 Mathomatic, so
they can be simplified, manipulated, and calculated.

Mathomatic input is filtered by m4, so opening a shell or an editor doesn't
work when running m4 Mathomatic. These are disabled.

The "quit" and "exit" commands may have some delay. You can use the EOF
character (control-D) to quit instantly, instead.

The read command currently doesn't use m4, so it can't process functions. The
way to read in text files with functions is to supply the filenames on the
shell command line:

	rmath filenames

All Mathomatic functions are real number, complex number, and symbolically
capable.

----------------------------------------------------------------------------

You can turn on color mode with the Mathomatic command:

	set color

For brighter colors, use:

	set bold color

typed at the Mathomatic main prompt.  For dimmer colors, type:

	set no bold

To turn off color mode, type:

	set no color

You can save the current color state (and all other settings) with:

	set save

so that Mathomatic starts up with your desired color setting every time.
