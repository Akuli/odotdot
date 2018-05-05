#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stddef.h>
#include "../context.h"         // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep


/* should
      * RETURN A NEW REFERENCE on success
      * set errptr and return NULL on error
      * usually not touch refcounts of args
      * not change the values of args
*/
typedef struct Object* (*functionobject_cfunc)(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs);

/* for example, do this in the beginning of a functionobject_cfunc...

	if (function_checktypes(ctx, errptr, args, nargs, "String", "Integer", "Object", NULL) == STATUS_ERROR)
		return STATUS_ERROR;

...to make sure that:
	* the function was called with 3 arguments
	* the first argument was a String
	* the second argument was an Integer

the types are looked up with interpreter_getbuiltin()
the 3rd argument can be anything because all classes inherit from Object
returns STATUS_OK or STATUS_ERROR
*/
int functionobject_checktypes(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs, ...);

// builtins_setup() sets interp->functionclass to this thing's return value
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_createclass(struct Interpreter *interp, struct Object **errptr);

// RETURNS A NEW REFERENCE or NULL on error
// if partialarg is not NULL, it's added as the first argument when the function is called
struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc);

// never fails
// TODO: remove this, functionobject_{v,}call() are better because partialled arguments
functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object *func);

// example: functionobject_call(ctx, errptr, func, a, b, c, NULL) calls func with arguments a, b, c
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Context *ctx, struct Object **errptr, struct Object *func, ...);

// named kinda like vprintf
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_vcall(struct Context *ctx, struct Object **errptr, struct Object *func, struct Object **args, size_t nargs);

// add a partial argument
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_newpartial(struct Interpreter *interp, struct Object **errptr, struct Object *func, struct Object *partialarg);


#endif   // OBJECTS_FUNCTION_H
