#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stddef.h>
#include "../objectsystem.h"    // IWYU pragma: keep


/* should
      * RETURN A NEW REFERENCE on success
      * set an error (see objects/errors.h) and return NULL on failure
      * NOT modify argarr or pass it to anything that may modify it
*/
typedef struct Object* (*functionobject_cfunc)(struct Interpreter *interp, struct Object *argarr);

/* for example, do this in the beginning of a functionobject_cfunc...

	if (function_checktypes(interp, argarr, interp->builtins.stringclass, interp->builtins.integerclass, interp->builtins.objectclass, NULL) == STATUS_ERROR)
		return NULL;

...to make sure that:
	* the function is being called with 3 arguments
	* the first argument is a String
	* the second argument is an Integer
	* the 3rd argument can be anything because all classes inherit from Object

bad things happen if the ... arguments are not class objects or you forget the NULL
returns STATUS_OK or STATUS_ERROR
*/
int functionobject_checktypes(struct Interpreter *interp, struct Object *argarr, ...);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_createclass(struct Interpreter *interp);

// returns STATUS_OK or STATUS_ERROR
int functionobject_addmethods(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
// if partialarg is not NULL, it's added as the first argument when the function is called
struct Object *functionobject_new(struct Interpreter *interp, functionobject_cfunc cfunc);

// example: functionobject_call(ctx, func, a, b, c, NULL) calls func with arguments a, b, c
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Interpreter *interp, struct Object *func, ...);

// named kinda like vprintf
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_vcall(struct Interpreter *interp, struct Object *func, struct Object *argarr);

// add a partial argument
// bad things happen if func is not a function object
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object *func, struct Object *partialarg);


#endif   // OBJECTS_FUNCTION_H
