#ifndef BUILTINS_ARRAYFUNC_H
#define BUILTINS_ARRAYFUNC_H

#include <stddef.h>
struct Interpreter;
struct Object;

struct Object *builtin_arrayfunc(struct Interpreter *interp, struct Object *argarr);

#endif    // BUILTINS_ARRAYFUNC_H
