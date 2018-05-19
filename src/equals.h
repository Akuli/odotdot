// TODO: get rid of this file and replace it with something nicer
#ifndef EQUALS_H
#define EQUALS_H

#include "interpreter.h"     // IWYU pragma: keep
#include "objectsystem.h"    // IWYU pragma: keep

// check if two objects should be considered equal
// unlike a == b, this does the right thing with e.g. Strings and Integers
// returns -1 on error
int equals(struct Interpreter *interp, struct Object **errptr, struct Object *a, struct Object *b);

#endif    // EQUALS_H
