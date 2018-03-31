#include <stdio.h>
#include <string.h>
#include <sys/time.h>    // for the posix-only and "obsolete" gettimeofday()
#include "utils.h"

typedef void (*testfunc)(void);
int verbose;

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
}
#define RUN_TEST(func) do { void func(void); run_test(#func, func); } while(0)


int main(int argc, char **argv)
{
	if (argc == 1)
		verbose = 0;
	else if (argc == 2 && strcmp(argv[1], "--verbose") == 0)
		verbose = 1;
	else {
		fprintf(stderr, "Usage: %s [--verbose]\n", argv[0]);
		return 1;
	}

	RUN_TEST(test_ast_node_structs_and_ast_copynode);
	RUN_TEST(test_ast_strings);
	RUN_TEST(test_ast_ints);
	RUN_TEST(test_ast_arrays);
	RUN_TEST(test_ast_getvars);
	RUN_TEST(test_ast_attributes);
	RUN_TEST(test_ast_function_call_statement);

	RUN_TEST(test_dynamiarray_new_push_pop_and_freeall);
	RUN_TEST(test_dynamiarray_equals_and_free);

	RUN_TEST(test_hashtable_basic_stuff);
	RUN_TEST(test_hashtable_many_values);
	RUN_TEST(test_hashtable_iterating);

	void objects_test_setup(void), objects_test_teardown(void);
	objects_test_setup();
	RUN_TEST(test_objects_objectclass_stuff);
	RUN_TEST(test_objects_simple);
	RUN_TEST(test_objects_function);
	RUN_TEST(test_objects_string);
	RUN_TEST(test_objects_classobject);
	objects_test_teardown();

	RUN_TEST(test_tokenizer_read_file_to_huge_string);
	RUN_TEST(test_tokenizer_tokenize);

	void unicode_test_setup(void);
	unicode_test_setup();
	RUN_TEST(test_utf8_encode);
	RUN_TEST(test_utf8_decode);

	if (verbose)
		printf("\n---------- all tests pass ----------\n");
	else
		printf("\nok\n");
	return 0;
}
