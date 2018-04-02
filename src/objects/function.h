#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include "../context.h"         // IWYU pragma: keep
#include "../dynamicarray.h"    // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep


// these should set errptr and return NULL on error
typedef struct Object* (*functionobject_cfunc)(struct Context *callctx, struct Object **errptr, struct Object **args, size_t nargs);

// sets interp->functionobjectinfo, returns STATUS_OK or STATUS_ERROR
int functionobject_createclass(struct Interpreter *interp, struct Object **errptr);

// returns NULL and sets errptr on error
struct Object *functionobject_new(struct Interpreter *interp, struct Object **errptr, functionobject_cfunc func);

// never fails
functionobject_cfunc functionobject_getcfunc(struct Interpreter *interp, struct Object *func);


#endif   // OBJECTS_FUNCTION_H
