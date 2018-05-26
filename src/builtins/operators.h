#ifndef BUILTIN_OPERATORS_H
#define BUILTIN_OPERATORS_H

struct Interpreter;
struct Object;

struct Object *builtin_equals(struct Interpreter *interp, struct Object *argarr);
struct Object *builtin_sameobject(struct Interpreter *interp, struct Object *argarr);

#endif    // BUILTIN_OPERATORS_H
