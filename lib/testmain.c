/*
 * This file contains the test/example program
 * for the Mathomatic symbolic math library and API.
 * Copy or refer to this,
 * if you are going to use the Mathomatic code in your other projects.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mathomatic.h"

int
main(int argc, char **argv)
{
	char		*cp;		/* character pointer */
	char		*ocp;		/* output character pointer */
	int		rv;		/* return value */
	char		buf[10000];	/* input buffer */
	char		*version;	/* version number of the library */

	printf("Mathomatic library test/example program.\n");
	/* Initialize all global variables and arrays so that Mathomatic will work properly. */
	if (!matho_init()) {	/* call this library function exactly once in your program */
		fprintf(stderr, "Not enough memory.\n");
		exit(1);
	}
	/* Mathomatic is ready for use. */
	if (matho_process((char *) "version", &version)) {
		printf("Mathomatic library version %s\n", version);
	} else {
		fprintf(stderr, "Error getting Symbolic Math Library version number.\n");
		fprintf(stderr, "Mathomatic version command failed.\n");
		exit(1);
	}

/* Uncomment the following if you would like "set save" to save the current session settings for every future session. */
/*	load_rc(true, NULL);	*/

	/* This is a standard input/output loop for testing. */
	printf("Press the EOF character (Control-D) to exit.\n");
	for (;;) {
		printf("%d-> ", cur_equation + 1);
		fflush(stdout);
		if ((cp = fgets(buf, sizeof(buf), stdin)) == NULL)
			break;
		/* Run the Mathomatic symbolic math engine. */
		rv = matho_process(cp, &ocp);
		if (warning_str) {
			/* Optionally display any warnings (not required, but helpful). */
			printf("Warning: %s\n", warning_str);
		}
		if (ocp) {
			if (rv && result_en >= 0) {
				/* Display the result equation number. */
				printf("%d: ", result_en + 1);
			}
			/* Display the result. */
			printf("Library result string:\n%s\n", ocp);
			if (rv) {
				free(ocp);
			} else {
				printf("Error return.\n");
			}
		}
	}
#if	VALGRIND
	free(version);
	free_mem();	/* reclaim all memory to check for memory leaks with something like valgrind(1) */
#endif
	printf("\n");
	exit(0);
}
