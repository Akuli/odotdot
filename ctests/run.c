#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>    // for the posix-only and "obsolete" gettimeofday()
#include "utils.h"

#include <src/builtins.h>
#include <src/gc.h>
#include <src/interpreter.h>
#include <src/run.h>

typedef void (*testfunc)(void);
bool verbose;
int ntests;


// this is externed in utils.h
struct Interpreter *testinterp;

static void run_test(char *name, testfunc func)
{
	buttert(!testinterp->err);

	if (verbose) {
		printf("%-50s  ", name);
		fflush(stdout);

		struct timeval start, end;
		buttert(gettimeofday(&start, NULL) == 0);
		func();
		buttert(!testinterp->err);
		buttert(gettimeofday(&end, NULL) == 0);
		unsigned long totaltime = (end.tv_sec - start.tv_sec)*1000*1000 + (end.tv_usec - start.tv_usec);
		printf("ok, %lu.%03lu ms\n", totaltime/1000, totaltime%1000);
	} else {
		func();
		buttert(!testinterp->err);
		printf(".");
		fflush(stdout);
	}
	ntests++;
}
#define RUN_TEST(func) do { void func(void); run_test(#func, func); buttert(!(testinterp->err)); } while(0)


int main(int argc, char **argv)
{
	if (argc == 1)
		verbose = false;
	else if (argc == 2 && (strcmp(argv[1], "--verbose") == 0 || strcmp(argv[1], "-v") == 0))
		verbose = true;
	else {
		fprintf(stderr, "Usage: %s [--verbose|-v]\n", argv[0]);
		return 1;
	}
	ntests = 0;

	buttert(testinterp = interpreter_new("testargv0"));
	buttert(builtins_setup(testinterp));
	buttert(run_builtinsfile(testinterp));

	RUN_TEST(test_objects_simple);
	RUN_TEST(test_objects_function);
	RUN_TEST(test_objects_string);
	RUN_TEST(test_objects_string_newfromfmt);
	RUN_TEST(test_objects_array_many_elems);
	RUN_TEST(test_objects_mapping_huge);
	RUN_TEST(test_objects_mapping_iter);

	RUN_TEST(test_integer_basic_stuff);

	RUN_TEST(test_ast_nodes_and_their_refcount_stuff);
	RUN_TEST(test_ast_strings);
	RUN_TEST(test_ast_ints);
	RUN_TEST(test_ast_arrays);
	RUN_TEST(test_ast_getvars);
	RUN_TEST(test_ast_attributes_and_methods);
	RUN_TEST(test_ast_function_call_statement);

	RUN_TEST(test_tokenizer_tokenize);

	builtins_teardown(testinterp);
	gc_run(testinterp);
	interpreter_free(testinterp);

	if (verbose)
		printf("\n---------------------------\nall %d ctests pass\n", ntests);
	else
		printf("\n");
	return 0;
}
