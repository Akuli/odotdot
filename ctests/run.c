#include <stdio.h>
#include <string.h>
#include <sys/time.h>    // for the posix-only and "obsolete" gettimeofday()
#include "utils.h"

#include <src/builtins.h>
#include <src/common.h>
#include <src/gc.h>
#include <src/interpreter.h>

typedef void (*testfunc)(void);
int verbose;
int ntests;
struct Interpreter *testinterp;   // externed in utils.h

static void run_test(char *name, testfunc func)
{
	if (verbose) {
		printf("   %-45s  ", name);
		fflush(stdout);

		struct timeval start, end;
		buttert(gettimeofday(&start, NULL) == 0);
		func();
		buttert(gettimeofday(&end, NULL) == 0);
		unsigned long totaltime = (end.tv_sec - start.tv_sec)*1000*1000 + (end.tv_usec - start.tv_usec);
		printf("ok, %lu.%03lu ms\n", totaltime/1000, totaltime%1000);
	} else {
		func();
		printf(".");
		fflush(stdout);
	}
	ntests++;
}
#define RUN_TEST(func) do { void func(void); run_test(#func, func); } while(0)


int main(int argc, char **argv)
{
	if (argc == 1)
		verbose = 0;
	else if (argc == 2 && (strcmp(argv[1], "--verbose") == 0 || strcmp(argv[1], "-v") == 0))
		verbose = 1;
	else {
		fprintf(stderr, "Usage: %s [--verbose|-v]\n", argv[0]);
		return 1;
	}
	ntests = 0;

	RUN_TEST(test_ast_node_structs_and_ast_copynode);
	RUN_TEST(test_ast_strings);
	RUN_TEST(test_ast_ints);
	RUN_TEST(test_ast_arrays);
	RUN_TEST(test_ast_getvars);
	RUN_TEST(test_ast_attributes_and_methods);
	RUN_TEST(test_ast_function_call_statement);

	RUN_TEST(test_dynamiarray_new_push_pop_and_freeall);
	RUN_TEST(test_dynamiarray_equals_and_free);

	RUN_TEST(test_hashtable_basic_stuff);
	RUN_TEST(test_hashtable_many_values);
	RUN_TEST(test_hashtable_iterating);

	buttert(testinterp = interpreter_new("testargv0"));
	buttert(builtins_setup(testinterp, NULL) == STATUS_OK);

	RUN_TEST(test_objects_objectclass_stuff);
	RUN_TEST(test_objects_simple);
	RUN_TEST(test_objects_function);
	RUN_TEST(test_objects_string);
	RUN_TEST(test_objects_string_tostring);
	RUN_TEST(test_objects_string_newfromfmt);
	RUN_TEST(test_objects_array);
	RUN_TEST(test_objects_integer);

	builtins_teardown(testinterp);
	interpreter_free(testinterp);

	RUN_TEST(test_tokenizer_tokenize);

	void unicode_test_setup(void); unicode_test_setup();
	RUN_TEST(test_utf8_encode);
	RUN_TEST(test_utf8_decode);

	if (verbose)
		printf("\n---------- all %d tests passed ----------\n", ntests);
	else
		printf("\nok\n");
	return 0;
}
