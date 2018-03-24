// minimal test framework

#ifndef TESTUTILS_H
#define TESTUTILS_H

// these have retarded comments on the side because iwyu doesn't like this file
#include <stdio.h>    // printf(), fprintf(), fflush(), stderr
#include <stdlib.h>   // abort(), exit()
#include <string.h>   // strcmp()
#include <ctype.h>    // isspace(), isalnum()
#include <sys/time.h>   // the "obsolete" and posix only gettimeofday()

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

void strip_spaces(char **s)
{
	while ((*s)[0] && isspace((*s)[0]))
		(*s)++;
}

int get_identifier(char **s, char *res)
{
	int i = 0;
	char c;
	while ((c=(*s)[i]) && (isalnum(c) || c == '_')) {
		res[i] = (*s)[i];
		i++;
	}
	res[i] = 0;
	*s += i;
	return i;
}

void do_the_test_run(int argc, char **argv, char *macrostring, testfunc *tests)
{
	int verbose;
	if (argc == 1)
		verbose = 0;
	else if (argc == 2 && strcmp(argv[1], "--verbose") == 0)
		verbose = 1;
	else {
		fprintf(stderr, "Usage: %s [--verbose]\n", argv[0]);
		exit(1);
	}

	if (verbose) {
		printf("%s:\n", argv[0]);
	} else {
		printf("%-35s  ", argv[0]);
		fflush(stdout);
	}

	// the macro magic creates a string like "test_foo, test_bar, test_baz"
	char testname[100];
	strip_spaces(&macrostring);
	while (get_identifier(&macrostring, testname)) {
		if (verbose) {
			printf("   %-45s  ", testname);
			fflush(stdout);
		}

		if (verbose) {
			struct timeval start, end;
			buttert(gettimeofday(&start, NULL) == 0);
			(tests[0])();
			buttert(gettimeofday(&end, NULL) == 0);
			unsigned long totaltime = (end.tv_sec - start.tv_sec)*1000*1000 + (end.tv_usec - start.tv_usec);
			printf("ok, %lu.%03lu ms\n", totaltime/1000, totaltime%1000);
		} else {
			(tests[0])();
			printf(".");
			fflush(stdout);
		}

		tests++;
		strip_spaces(&macrostring);
		if (macrostring[0] == ',') {
			macrostring++;
			strip_spaces(&macrostring);
		}
	}

	printf("\n");
}

#define RUN_TESTS(argc, argv, ...) do_the_test_run(argc, argv, #__VA_ARGS__, ((testfunc[]){__VA_ARGS__, NULL}))
#define TESTS_MAIN(...) int main(int argc, char **argv) { RUN_TESTS(argc, argv, __VA_ARGS__); return 0; }

#endif   // TESTUTILS_H
