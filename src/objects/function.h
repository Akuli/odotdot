#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stddef.h>
#include "../context.h"         // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep


/* should
      * RETURN A REFERENCE on success
      * set errptr and return NULL on error
      * not touch refcounts of args
      * not change the values of args
*/
typedef struct Object* (*functionobject_cfunc)(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs);

// sets interp->functionobjectinfo, returns STATUS_OK or STATUS_ERROR
int functionobject_createclass(struct Interpreter *interp, struct Object **errptr);

// returns NULL and sets errptr on error
// RETURNS A NEW REFERENCE
struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc cfunc);

// never fails
functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object *func);

// example: functionobject_call(ctx, errptr, func, a, b, c, NULL) calls func with arguments a, b, c
// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_call(struct Context *ctx, struct Object **errptr, struct Object *func, ...);


#endif   // OBJECTS_FUNCTION_H
