#ifndef BUILTINS_OPERATORS_H
#define BUILTINS_OPERATORS_H

struct Interpreter;
struct Object;

struct Object *builtin_equals(struct Interpreter *interp, struct Object *argarr);
struct Object *builtin_sameobject(struct Interpreter *interp, struct Object *argarr);

#endif    // BUILTINS_OPERATORS_H
