#include <stdio.h>
#include <string.h>
#include <sys/time.h>    // for the posix-only and "obsolete" gettimeofday()
#include "utils.h"

#include <src/common.h>
#include <src/objects/classobject.h>
#include <src/objects/errors.h>
#include <src/objects/object.h>
#include <src/objects/string.h>

typedef void (*testfunc)(void);
int verbose;
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
}
#define RUN_TEST(func) do { void func(void); run_test(#func, func); } while(0)


/*void objects_test_setup(void)
{
	buttert(objectclass = objectobject_createclass());
	buttert(functionclass = functionobject_createclass(objectclass));
	buttert(stringclass = stringobject_createclass(objectclass));
	buttert(classobjectclass = classobject_createclass(objectclass));
}
void objects_test_teardown(void)
{
	objectclassinfo_free(classobjectclass);
	objectclassinfo_free(stringclass);
	objectclassinfo_free(functionclass);
	objectclassinfo_free(objectclass);
}*/
// TODO: get rid of these stupid globals
struct ObjectClassInfo *objectinfo, *errorinfo, *stringinfo;
static void setup_testinterp(void) {
	buttert(testinterp = interpreter_new("testargv0"));
	buttert(objectinfo = objectobject_createclass());
	buttert(errorinfo = errorobject_createclass(objectinfo));
	buttert(stringinfo = stringobject_createclass(objectinfo));
	buttert(testinterp->nomemerr = errorobject_newfromcharptr(errorinfo, stringinfo, "not enough memory"));

	// now we can use errptr, but tests pass NULL for errptr when errors are not welcome
	buttert(testinterp->classobjectinfo = classobject_createclass(testinterp, NULL, objectinfo));
	buttert(interpreter_addbuiltin(testinterp, NULL, "Object", classobject_newfromclassinfo(testinterp, NULL, objectinfo)) == STATUS_OK);
}
static void teardown_testinterp(void) {
	object_free(interpreter_getbuiltin(testinterp, NULL, "Object"));
	objectclassinfo_free(testinterp->classobjectinfo);
	object_free(testinterp->nomemerr->data);   // the message string
	object_free(testinterp->nomemerr);
	objectclassinfo_free(stringinfo);
	objectclassinfo_free(errorinfo);
	objectclassinfo_free(objectinfo);
	interpreter_free(testinterp);
}


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

	setup_testinterp();
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

	RUN_TEST(test_objects_objectclass_stuff);
	RUN_TEST(test_objects_simple);
	//RUN_TEST(test_objects_function);   // TODO: do functions well
	//RUN_TEST(test_objects_string);   // TODO: replace passing in ObjectClassInfo with interp or something

	RUN_TEST(test_tokenizer_read_file_to_huge_string);
	RUN_TEST(test_tokenizer_tokenize);

	void unicode_test_setup(void); unicode_test_setup();
	RUN_TEST(test_utf8_encode);
	RUN_TEST(test_utf8_decode);

	teardown_testinterp();

	if (verbose)
		printf("\n---------- all tests pass ----------\n");
	else
		printf("\nok\n");
	return 0;
}
