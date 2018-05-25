#ifndef BUILTINS_PRINT_H
#define BUILTINS_PRINT_H

#include <stddef.h>
struct Interpreter;
struct Object;

struct Object *builtin_print(struct Interpreter *interp, struct Object **args, size_t nargs);

#endif  // BUILTINS_PRINT_H
