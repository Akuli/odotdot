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
typedef struct Object* (*functionobject_cfunc)(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs, void *data);

// sets interp->functionobjectinfo, returns STATUS_OK or STATUS_ERROR
int functionobject_createclass(struct Interpreter *interp, struct Object **errptr);

// returns NULL and sets errptr on error
// RETURNS A NEW REFERENCE
struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc, void *data);

// never fails
// TODO: remove this, functionobject_{v,}call() are better because the data
functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object *func);

// example: functionobject_call(ctx, errptr, func, a, b, c, NULL) calls func with arguments a, b, c
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Context *ctx, struct Object **errptr, struct Object *func, ...);

// named kinda like vprintf
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_vcall(struct Context *ctx, struct Object **errptr, struct Object *func, struct Object **args, size_t nargs);


#endif   // OBJECTS_FUNCTION_H
