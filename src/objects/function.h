#ifndef OBJECTS_FUNCTION_H
#define OBJECTS_FUNCTION_H

#include <stdbool.h>
#include "../objectsystem.h"

// interpreter.h includes this
// and stupid IWYU hates this and has no way to unhate this....
struct Interpreter;


/* should
	* return true on success
	* throw an error and return false on failure
	* NOT modify args or pass it to anything that may modify it

args is an Array object of arguments
opts is a Mapping of options, keys are strings and values are Objects
userdata is the ObjectData given when creating the function object
some things pass (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL} for userdata
*/
typedef bool (*functionobject_cfunc_noret)(struct Interpreter *interp, struct ObjectData userdata, struct Object *args, struct Object *opts);

// like functionobject_cfunc_noret, but should always return a new reference, or NULL on error
typedef struct Object* (*functionobject_cfunc_yesret)(struct Interpreter *interp, struct ObjectData userdata, struct Object *args, struct Object *opts);

// just because these are passed around a lot
struct FunctionObjectCfunc {
	bool returning;
	struct {
		functionobject_cfunc_yesret yesret;
		functionobject_cfunc_noret noret;
	} func;
};

// convenience functions for defining FunctionObjectCfuncs, these never fails
struct FunctionObjectCfunc functionobject_mkcfunc_yesret(functionobject_cfunc_yesret func);
struct FunctionObjectCfunc functionobject_mkcfunc_noret(functionobject_cfunc_noret func);


// RETURNS A NEW REFERENCE or NULL on error
struct Object *functionobject_createclass(struct Interpreter *interp);

// returns false on error
bool functionobject_addmethods(struct Interpreter *interp);

// these RETURN A NEW REFERENCE or NULL on error
struct Object *functionobject_new(struct Interpreter *interp, struct ObjectData userdata, struct FunctionObjectCfunc cfunc, char *name);

// creates a Function object and adds it to an Array object
// especially useful with interp->oparrays
// passes (struct ObjectData){.data=NULL, .foreachref=NULL, .destructor=NULL} for userdata
// returns false on error
bool functionobject_add2array(struct Interpreter *interp, struct Object *arr, char *name, struct FunctionObjectCfunc cfunc);

// example: functionobject_call(interp, func, a, b, c, NULL) calls func with arguments a, b, c
// bad things happen if func is not a function object or you forget the NULL
// an error is thrown and NULL or false is returned if func is of the wrong kind (returning / not returning)
// similar return values and error handling as with functionobject_cfunc_{yes,no}ret
struct Object *functionobject_call_yesret(struct Interpreter *interp, struct Object *func, ...);
bool functionobject_call_noret(struct Interpreter *interp, struct Object *func, ...);

// see functionobject_call_{yes,no}ret, these are named kinda like vprintf
struct Object *functionobject_vcall_yesret(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts);
bool functionobject_vcall_noret(struct Interpreter *interp, struct Object *func, struct Object *args, struct Object *opts);

// throws an error and returns false on failure
// bad things happen if func is not a Function object
bool functionobject_setname(struct Interpreter *interp, struct Object *func, char *newname);


#endif   // OBJECTS_FUNCTION_H
