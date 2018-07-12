#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <stdbool.h>
#include <stddef.h>
#include "check.h"   // for the ATTRIBUTE_DEFINE_blah macros
#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"   // IWYU pragma: keep
#include "unicode.h"        // IWYU pragma: keep
#include "objects/array.h"
#include "objects/function.h"

// returns false on error
// setter and getter can be NULL (but not both, that would do nothing)
// setter is called with 2 arguments: the object, new value
// getter is called with 1 argument: the object
// setter and getter are called with (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL}
// bad things happen if klass is not a class object, name is very long or you don't check_args() in getter and setter
bool attribute_add(struct Interpreter *interp, struct Object *klass, char *name, functionobject_cfunc_yesret getter, functionobject_cfunc_noret setter);
bool attribute_addwithfuncobjs(struct Interpreter *interp, struct Object *klass, char *name, struct Object *getter, struct Object *setter);

// these RETURN A NEW REFERENCE or NULL on error
struct Object *attribute_get(struct Interpreter *interp, struct Object *obj, char *attr);
struct Object *attribute_getwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj);

// these return false on error
bool attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);
bool attribute_setwithstringobj(struct Interpreter *interp, struct Object *obj, struct Object *stringobj, struct Object *val);

// convenience functions that call mappingobject_set() and mappingobject_get() with obj->attrdata, initializing it if needed
// setdata returns false on error, getdata returns a new reference or NULL
bool attribute_settoattrdata(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);
struct Object *attribute_getfromattrdata(struct Interpreter *interp, struct Object *obj, char *attr);

// returns false on error
bool attribute_set(struct Interpreter *interp, struct Object *obj, char *attr, struct Object *val);


// because writing getters sucks
#define ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(CLASSNAME, STRUCTNAME, MEMBERNAME) \
static struct Object *MEMBERNAME##_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts) \
{ \
	if (!check_args(interp, args, interp->builtins.CLASSNAME, NULL)) return NULL; \
	if (!check_no_opts(interp, opts)) return NULL; \
	struct STRUCTNAME data = *(struct STRUCTNAME*) ARRAYOBJECT_GET(args, 0)->objdata.data; \
	OBJECT_INCREF(interp, data.MEMBERNAME); \
	return data.MEMBERNAME; \
}

#endif     // ATTRIBUTE_H
