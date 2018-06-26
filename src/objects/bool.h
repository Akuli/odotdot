// see bool.h for explanation of why this is not implemented in pure ö

#ifndef OBJECTS_BOOL_H
#define OBJECTS_BOOL_H

#include <stdbool.h>

struct Interpreter;
struct Object;

// sets yes and no to Ö true and false, as new references
// returns a C stdbool that represents success
// yes and no are left untouched on error
bool boolobject_create(struct Interpreter *interp, struct Object **yes, struct Object **no);

// builtins.c calls this
bool boolobject_initoparrays(struct Interpreter *interp);

#endif    // OBJECTS_BOOL_H
