// this is not a part of builtins.c because this is more code than most other builtins
#ifndef LAMBDABUILTIN_H
#define LAMBDABUILTIN_H

struct Interpreter;
struct Object;

struct Object *lambdabuiltin(struct Interpreter *interp, struct Object *args, struct Object *opts);

#endif    // LAMBDABUILTIN_H
