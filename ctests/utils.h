// minimal test framework

#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BEGIN_TESTS int main(void) { char TEST_NAME[100];
#define TEST(name) strcpy(TEST_NAME, #name); printf("   testing %s:%s\n", __FILE__, #name);
#define END_TESTS return 0; }

// these are not called assert because assert would conflict with assert.h
// instead, replace ASS in ASSert with BUTT
#define buttert2(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "buttertion '%s' failed in %s:%d, test '%s': %s\n", \
			#cond, __FILE__, __LINE__, TEST_NAME, msg); \
		abort(); \
	} \
} while (0)
#define buttert(cond) buttert2(cond, "")

#endif   // TESTUTILS_H
