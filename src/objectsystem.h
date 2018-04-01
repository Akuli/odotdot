#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include "hashtable.h"     // IWYU pragma: keep
#include "unicode.h"       // IWYU pragma: keep


// iwyu is stupid, why can't i pragma keep structs forward declarations :(
struct Object;   // forward declaration

typedef void (*objectclassinfo_foreachrefcb)(struct Object *obj, void *data);
typedef void (*objectclassinfo_foreachref)(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb);

// every ö class is represented as an ObjectClassInfo struct
// these are created by whateverobject_createclass() functions
struct ObjectClassInfo {
	// Object's baseclass is set to NULL
	struct ObjectClassInfo *baseclass;

	// keys are UnicodeStrings, values are Function objects (see objects/function.{c,h})
	struct HashTable *methods;

	// calls cb(ref, data) for each ref object that this object refers to
	// this is used for garbage collecting
	// can be NULL
	objectclassinfo_foreachref foreachref;

	// called by object_free(), should return STATUS_OK or STATUS_NOMEM
	// can be NULL
	void (*destructor)(struct Object *obj);
};

// all ö objects are instances of this struct
struct Object {
	struct ObjectClassInfo *klass;

	// keys are UnicodeStrings pointers, values are pointers to Function objects
	struct HashTable *attrs;

	// NULL for objects created from the language, but constructor functions
	// written in C can use this for anything
	void *data;

	// the garbage collector does something implementation-detaily with this
	int gcflag;
};

// helper for blabla_createclass() functions, returns NULL on no mem
struct ObjectClassInfo *objectclassinfo_new(struct ObjectClassInfo *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *));

// never fails
void objectclassinfo_free(struct ObjectClassInfo *klass);

// some convenience methods
// returns STATUS_NOMEM or a return value from hashtable_set
int objectsystem_addbuiltinclass(struct HashTable *builtins, char *name, struct ObjectClassInfo *klass);

// returns STATUS_OK, STATUS_NOMEM or 1 for not found, assert(0)'s if name is not utf8
// TODO: struct Object* instead of void*
int objectsystem_getbuiltin(struct HashTable *builtins, char *name, void **res);

// see also classobject_newinstance() in objects/classobject.h
struct Object *object_new(struct ObjectClassInfo *klass, void *data);

void object_free(struct Object *obj);

// returns a Function object, or NULL if not found
struct Object *object_getmethod(struct ObjectClassInfo *klass, struct UnicodeString name);


#endif   // OBJECTSYSTEM_H
