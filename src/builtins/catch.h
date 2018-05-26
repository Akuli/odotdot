#ifndef BUILTINS_CATCH_H
#define BUILTINS_CATCH_H

struct Interpreter;
struct Object;

struct Object *builtin_catch(struct Interpreter *interp, struct Object *argarr);

#endif    // BUILTINS_CATCH_H
