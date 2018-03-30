#ifndef OBJECTSYSTEM_H
#define OBJECTSYSTEM_H

#include "hashtable.h"     // needed for HashTable, iwyu doesn't get it
#include "unicode.h"     // needed for UnicodeString, iwyu doesn't get it


struct Object;   // forward declaration

typedef int (*objectclassinfo_destructor_t)(struct Object *obj);

// every ö class is represented as an ObjectClassInfo struct
// these are created by whateverobject_createclass() functions
struct ObjectClassInfo {
	// Object's baseclass is set to NULL
	struct ObjectClassInfo *baseclass;

	// keys are UnicodeStrings, values are Function objects (see objects/function.{c,h})
	struct HashTable *methods;

	// called by object_free(), should return STATUS_OK or STATUS_NOMEM
	// can be NULL
	objectclassinfo_destructor_t destructor;
};

// all ö objects are instances of this struct
struct Object {
	struct ObjectClassInfo *klass;

	// keys are UnicodeStrings pointers, values are pointers to Function objects
	struct HashTable *attrs;

	// NULL for objects created from the language, but constructor functions
	// written in C can use this for anything
	void *data;
};

// helper for blabla_createclass() functions, returns NULL on no mem
struct ObjectClassInfo *objectclassinfo_new(struct ObjectClassInfo *base, objectclassinfo_destructor_t destructor);

// never fails
void objectclassinfo_free(struct ObjectClassInfo *klass);

// some convenience methods
// returns STATUS_NOMEM or a return value from hashtable_set
int objectsystem_addbuiltinclass(struct HashTable *builtins, char *name, struct ObjectClassInfo *klass);

// returns STATUS_OK, STATUS_NOMEM or 1 for not found, assert(0)'s if name is not utf8
// TODO: struct Object* instead of void*
int objectsystem_getbuiltin(struct HashTable *builtins, char *name, void **res);

// this does not call the ö setup method, call it yourself if you want to
// when an object is created from ö, this is called, followed by a setup()
// this sets data to NULL
struct Object *object_new(struct ObjectClassInfo *klass);

// returns STATUS_something, but always frees the object making it unusable
int object_free(struct Object *obj);

// returns a Function object, or NULL if not found
struct Object *object_getmethod(struct ObjectClassInfo *klass, struct UnicodeString name);


#endif   // OBJECTSYSTEM_H
