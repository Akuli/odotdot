#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include "object.h"

typedef int (*function_cfunc)(struct Object *ctx, struct Object **retval);

int function_init_class(struct ObjectClassInfo *klass);


#endif   // OBJECTS_FUNCTION_H
