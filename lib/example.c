/*
 * Real quick and dirty example of usage of the Mathomatic Symbolic Math Library.
 * Designed to be as simple as possible, yet work.
 * Just displays: "x = 2*sign".
 */
#include <stdio.h>
#include <stdlib.h>
#include "mathomatic.h"

int
main()
{
#if	WANT_LEAKS	/* Causes memory leaks, wrong code to use, but will work in a pinch. */
    char *output;

    matho_init();
    matho_parse("x^2=4", NULL);
    matho_process("solve x", &output);
    printf("%s\n", output); 
#else			/* right code to use */
    char *output;
    int rv;

    if (!matho_init()) {
        printf("Not enough memory.\n");
        exit(1);
    }
    matho_parse((char *) "x^2=4", NULL);
    rv = matho_process((char *) "solve x", &output);
    if (output) {
        printf("%s\n", output);
        if (rv) {
            free(output);
        } else {
            printf("Error return.\n");
        }
    }
#endif

    exit(0);
}
