#ifndef OBJECTS_BYTEARRAY_H
#define OBJECTS_BYTEARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

struct ByteArrayObjectData {
	unsigned char *val;
	size_t len;
};

// RETURNS A NEW REFERENCE or NULL on error
struct Object *bytearrayobject_createclass(struct Interpreter *interp);

// for builtins_setup()
bool bytearrayobject_initoparrays(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
// data should be allocated with malloc and should NOT be modified or freed after calling this
// use bytearrayobject_new(interp, NULL, 0) to create an empty ByteArray
struct Object *bytearrayobject_new(struct Interpreter *interp, unsigned char *val, size_t len);

// bad things happen if arr is not an array object, otherwise these never fail
#define BYTEARRAYOBJECT_DATA(arr) (((struct ByteArrayObjectData *) (arr)->objdata.data)->val)
#define BYTEARRAYOBJECT_LEN(arr)  (((struct ByteArrayObjectData *) (arr)->objdata.data)->len)

// RETURNS A NEW REFERENCE or NULL on error
struct Object *bytearrayobject_slice(struct Interpreter *interp, struct Object *arr, long long start, long long end);

#endif    // OBJECTS_BYTEARRAY_H
