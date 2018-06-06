#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"   // IWYU pragma: keep
#include "unicode.h"        // IWYU pragma: keep
#include "objects/array.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/null.h"

// returns STATUS_OK or STATUS_ERROR
// setter and getter can be NULL (but not both, that would do nothing)
// bad things happen if klass is not a class object
int attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc getter, functionobject_cfunc setter);
int attribute_addwithfuncobjs(struct Interpreter *interp, struct Object *klass, char *name, struct Object *getter, struct Object *setter);

// these RETURN A NEW REFERENCE or NULL on error
struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr);
struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj);

// these return STATUS_OK or STATUS_ERROR
int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);
int attribute_setwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj, struct Object *val);

// convenience functions that call mappingobject_set() and mappingobject_get() with obj->attrdata
// setdata returns STATUS_OK or STATUS_ERROR, getdata returns a new reference or NULL
int attribute_settoattrdata(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);
struct Object *attribute_getfromattrdata(struct Interpreter *interp, struct Object *obj, char *attr);

// returns STATUS_OK or STATUS_ERROR
int attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);


// because writing unnecessary getters and setters by hand sucks, java users know it
// these are meant to be placed outside other functions
#define ATTRIBUTE_DEFINE_SIMPLE_GETTER(ATTRNAME, CLASSNAME) \
	static struct Object* ATTRNAME##_getter(struct Interpreter *interp, struct Object *argarr) \
	{ \
		if (check_args(interp, argarr, interp->builtins.CLASSNAME, NULL) == STATUS_ERROR) \
			return NULL; \
		return attribute_getfromattrdata(interp, ARRAYOBJECT_GET(argarr, 0), #ATTRNAME); \
	}

#define ATTRIBUTE_DEFINE_SIMPLE_SETTER(ATTRNAME, CLASSNAME, VALUECLASSNAME) \
	static struct Object* ATTRNAME##_setter(struct Interpreter *interp, struct Object *argarr) \
	{ \
		if (check_args(interp, argarr, interp->builtins.CLASSNAME, interp->builtins.VALUECLASSNAME, NULL) == STATUS_ERROR) \
			return NULL; \
		return attribute_settoattrdata(interp, ARRAYOBJECT_GET(argarr, 0), #ATTRNAME, ARRAYOBJECT_GET(argarr, 1))==STATUS_OK \
			? nullobject_get(interp) : NULL; \
	}


#endif     // ATTRIBUTE_H
