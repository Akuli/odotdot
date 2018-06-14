#ifndef STACK_H
#define STACK_H

// allow setting this with -DSTACK_MAX in case someone wants to have a bigger stack
// python's default is 1000, and everyone seem to be fine with it most of the time
#ifndef STACK_MAX
#define STACK_MAX 1000
#endif

#include <stdbool.h>
#include <stddef.h>

// interpreter.h includes this
struct Interpreter;


struct StackFrame {
	char *filename;         // absolute or relative, should probably switch to absolute paths
	size_t lineno;          // 1 for first line, must not be zero
	struct Object *scope;   // the scope that the code is running in, or NULL
};

// sets an error and returns false on failure
bool stack_push(struct Interpreter *interp, char *filename, size_t lineno, struct Object *scope);

// aborts if the stack is empty
void stack_pop(struct Interpreter *interp);

#endif   // STACK_H
