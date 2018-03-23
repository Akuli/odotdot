// minimal test framework

#ifndef TESTUTILS_H
#define TESTUTILS_H

// these have retarded comments on the side because iwyu doesn't like this file
#include <stdio.h>    // printf(), fprintf(), fflush()
#include <stdlib.h>   // abort()

// these are not called assert because assert would conflict with assert.h
// instead, replace ASS in ASSert with BUTT
#define buttert2(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "buttertion '%s' failed (%s:%d, func '%s'): %s\n", \
			#cond, __FILE__, __LINE__, __func__, (msg)); \
		abort(); \
	} \
} while (0)
#define buttert(cond) buttert2(cond, "")

void *bmalloc(size_t size)
{
	void *res = malloc(size);
	buttert2(res, "not enough mem :(");
	return res;
}


typedef void (*testfunc)(void);

void run_tests(char *progname, testfunc *tests)
{
	printf("%-35s  ", progname);
	fflush(stdout);
	for (int i=0; tests[i]; i++) {
		(tests[i])();
		printf(".");
		fflush(stdout);
	}
	printf("\n");
}

#define TESTS_MAIN(...) int main(int argc, char **argv) { run_tests(argv[0], ((testfunc[]){__VA_ARGS__})); return 0; }

#endif   // TESTUTILS_H
