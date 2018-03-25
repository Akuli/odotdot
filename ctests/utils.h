// fun test framework

#ifndef TESTUTILS_H
#define TESTUTILS_H

// these have retarded comments on the side because iwyu doesn't like this file
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>     // for abort(), iwyu doesn't get it

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

void *bmalloc(size_t size);

#endif   // TESTUTILS_H
