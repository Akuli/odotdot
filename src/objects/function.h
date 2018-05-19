#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stddef.h>
#include "../objectsystem.h"    // IWYU pragma: keep


/* should
      * RETURN A NEW REFERENCE on success
      * set errptr and return NULL on error
      * usually not touch refcounts of args
      * not change the values of args
*/
typedef struct Object* (*functionobject_cfunc)(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs);

/* for example, do this in the beginning of a functionobject_cfunc...

	if (function_checktypes(interp, errptr, args, nargs, interp->builtins.stringclass, interp->builtins.integerclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;

...to make sure that:
	* the function is being called with 3 arguments
	* the first argument is a String
	* the second argument is an Integer

the 3rd argument can be anything because all classes inherit from Object
bad things happen if the ... arguments are not class objects or NULL
returns STATUS_OK or STATUS_ERROR
*/
int functionobject_checktypes(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs, ...);

// builtins_setup() sets interp->functionclass to this thing's return value
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_createclass(struct Interpreter *interp, struct Object **errptr);

// RETURNS A NEW REFERENCE or NULL on error
// if partialarg is not NULL, it's added as the first argument when the function is called
struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc);

// example: functionobject_call(ctx, errptr, func, a, b, c, NULL) calls func with arguments a, b, c
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Interpreter *interp, struct Object **errptr, struct Object *func, ...);

// named kinda like vprintf
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_vcall(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object **args, size_t nargs);

// add a partial argument
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object *partialarg);


#endif   // OBJECTS_FUNCTION_H
