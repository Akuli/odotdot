#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stddef.h>
#include "../objectsystem.h"    // IWYU pragma: keep

// interpreter.h includes this
struct Interpreter;


/* should
      * RETURN A NEW REFERENCE on success
      * set an error (see objects/errors.h) and return NULL on failure
      * NOT modify args or pass it to anything that may modify it

args is an Array object of arguments
opts is a Mapping of options, keys are strings and values are Objects
*/
typedef struct Object* (*functionobject_cfunc)(struct Interpreter *interp, struct Object *args, struct Object *opts);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_createclass(struct Interpreter *interp);

// returns false on error
bool functionobject_addmethods(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
// if partialarg is not NULL, it's added as the first argument when the function is called
struct Object *functionobject_new(struct Interpreter *interp, functionobject_cfunc cfunc, char *name);

// creates a Function object and adds it to an Array object
// especially useful with interp->oparrays
// returns false on error
bool functionobject_add2array(struct Interpreter *interp, struct Object *arr, char *name, functionobject_cfunc cfunc);

// add a partial argument
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object *func, struct Object *partialarg);

// example: functionobject_call(ctx, func, a, b, c, NULL) calls func with arguments a, b, c
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Interpreter *interp, struct Object *func, ...);

// named kinda like vprintf
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_vcall(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts);

// returns false on error
// bad things happen if func is not a Function object
bool functionobject_setname(struct Interpreter *interp, struct Object *func, char *newname);


#endif   // OBJECTS_FUNCTION_H
