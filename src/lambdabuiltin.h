// this is not a part of builtins.c because this is more code than most other builtins
#ifndef LAMBDABUILTIN_H
#define LAMBDABUILTIN_H

#include "objectsystem.h"

struct Object *lambdabuiltin(struct Interpreter *interp, struct ObjectData dummydata, struct Object *args, struct Object *opts);

#endif    // LAMBDABUILTIN_H
