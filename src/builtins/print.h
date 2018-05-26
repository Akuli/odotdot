#ifndef BUILTINS_PRINT_H
#define BUILTINS_PRINT_H

#include <stddef.h>
struct Interpreter;
struct Object;

struct Object *builtin_print(struct Interpreter *interp, struct Object *argarr);

#endif  // BUILTINS_PRINT_H
