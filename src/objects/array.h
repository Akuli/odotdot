#ifndef OBJECTS_ARRAY_H
#define OBJECTS_ARRAY_H

#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

struct ArrayObjectData {
	size_t len;
	size_t nallocated;    // implementation detail
	struct Object **elems;
};

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_createclass(struct Interpreter *interp, struct Object **errptr);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_new(struct Interpreter *interp, struct Object **errptr, struct Object **elems, size_t nelems);

// for convenience
#define arrayobject_newempty(interp, errptr) arrayobject_new((interp), (errptr), NULL, 0)

// bad things happen if arr is not an array object or i < ARRAYOBJECT_LEN(arr)
// otherwise these never fail
// note that ARRAYOBJECT_GET does NOT return a new reference!
#define ARRAYOBJECT_GET(arr, i) (((struct ArrayObjectData *) (arr)->data)->elems[(i)])
#define ARRAYOBJECT_LEN(arr)    (((struct ArrayObjectData *) (arr)->data)->len)

// returns STATUS_OK or STATUS_ERROR
// bad things happen if arr is not an array object
int arrayobject_push(struct Interpreter *interp, struct Object **errptr, struct Object *arr, struct Object *elem);

// RETURNS A NEW REFERENCE, or NULL if the array is empty
// never fails if arr is an array object, bad things happen if it isn't
struct Object *arrayobject_pop(struct Interpreter *interp, struct Object *arr);

#endif    // OBJECTS_ARRAY_H
