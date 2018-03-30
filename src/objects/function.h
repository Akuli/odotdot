#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include "../objectsystem.h"    // IWYU pragma: keep


// these should return STATUS_OK or STATUS_NOMEM
typedef int (*functionobject_cfunc)(struct Object **retval);

struct ObjectClassInfo *functionobject_createclass(struct ObjectClassInfo *objectclass);
struct Object *functionobject_new(struct ObjectClassInfo *functionclass, functionobject_cfunc func);
functionobject_cfunc functionobject_get_cfunc(struct Object *funcobj);


#endif   // OBJECTS_FUNCTION_H
