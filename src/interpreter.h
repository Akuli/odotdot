#ifndef INTERPRETER_H
#define INTERPRETER_H
 
#include "hashtable.h"    // IWYU pragma: keep

// these are defined in other files that need to include this file
// stupid IWYU doesn't get this.....
struct Context;
struct Object;

struct InterpreterBuiltins {
	struct Object *arrayclass;
	struct Object *astnodeclass;
	struct Object *classclass;    // the class of class objects
	struct Object *errorclass;
	struct Object *functionclass;
	struct Object *integerclass;
	struct Object *mappingclass;
	struct Object *objectclass;
	struct Object *stringclass;

	struct Object *nomemerr;
	struct Object *print;
};

struct Interpreter {
	// this is set to argv[0] from main(), useful for error messages
	char *argv0;

	// some builtins are also available here (e.g. String), others (e.g. AstNode) are not
	struct Context *builtinctx;

	// keys are all objects
	// values are pointers to a static dummy variable
	// hashes are (unsigned int)((uintptr_t)obj)
	// see also objectsystem.h
	// this doesn't hold any references, object_free_impl() takes care of that
	struct HashTable *allobjects;

	// this holds references to built-in classes, functions and stuff
	struct InterpreterBuiltins builtins;
};

// returns NULL on no mem
// pretty much nothing is ready after calling this, use builtins_setup()
struct Interpreter *interpreter_new(char *argv0);

// never fails
void interpreter_free(struct Interpreter *interp);

// convenience methods, these assert that name is valid UTF-8

// returns STATUS_OK or STATUS_ERROR
int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val);

// returns NULL and sets errptr on error
// RETURNS A NEW REFERENCE
struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name);

/*
simple version of getbuiltin() that never calls malloc()
useful in e.g. builtins_teardown()
restrictions:
	* strlen(name) must be < 50
	* name must be ASCII only
	* no errptr, instead returns NULL if the variable doesn't exist
RETURNS A NEW REFERENCE
*/
struct Object *interpreter_getbuiltin_nomalloc(struct Interpreter *interp, char *name);

#endif   // INTERPRETER_H
