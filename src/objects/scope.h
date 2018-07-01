#ifndef OBJECTS_SCOPE_H
#define OBJECTS_SCOPE_H

#include "../interpreter.h"     // IWYU pragma: keep
#include "../objectsystem.h"    // IWYU pragma: keep

struct ScopeObjectData {
	struct Object *parent_scope;   // NULL for the built-in scope
	struct Object *local_vars;
};

#define SCOPEOBJECT_PARENTSCOPE(obj) ((struct ScopeObjectData *) (obj)->data)->parent_scope
#define SCOPEOBJECT_LOCALVARS(obj) ((struct ScopeObjectData *) (obj)->data)->local_vars

// RETURNS A NEW REFERENCE or NULL on error
struct Object *scopeobject_createclass(struct Interpreter *interp);

// RETURNS A NEW REFERENCE
struct Object *scopeobject_newbuiltin(struct Interpreter *interp);

// bad things happen if parent_scope is not a scope object
// RETURNS A NEW REFERENCE
struct Object *scopeobject_newsub(struct Interpreter *interp, struct Object *parent_scope);

// varname must be a String object
// bad things happen if scope is not a Scope object
bool scopeobject_setvar(struct Interpreter *interp, struct Object *scope, struct Object *varname, struct Object *val);
struct Object *scopeobject_getvar(struct Interpreter *interp, struct Object *scope, struct Object *varname);

#endif   // OBJECTS_SCOPE_H
