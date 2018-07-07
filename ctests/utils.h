#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <stddef.h>
#include <stdio.h>

// iwyu doesn't understand these, so check them yourself when iwyuing
#include <stdlib.h>            // IWYU pragma: keep
#include <src/interpreter.h>   // IWYU pragma: keep

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

extern struct Interpreter *testinterp;

#endif   // TESTUTILS_H
