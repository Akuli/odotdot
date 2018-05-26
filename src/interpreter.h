#ifndef INTERPRETER_H
#define INTERPRETER_H
 
#include "allobjects.h"

// these are defined in other files that need to include this file
// stupid IWYU doesn't get this.....
struct Context;
struct Object;

struct InterpreterBuiltins {
	struct Object *arrayclass;
	struct Object *astnodeclass;
	struct Object *blockclass;
	struct Object *classclass;    // the class of class objects
	struct Object *errorclass;
	struct Object *functionclass;
	struct Object *integerclass;
	struct Object *mappingclass;
	struct Object *objectclass;
	struct Object *scopeclass;
	struct Object *stringclass;

	struct Object *array_func;
	struct Object *catch;
	struct Object *equals;
	struct Object *nomemerr;
	struct Object *print;
	struct Object *same_object;
};

struct Interpreter {
	// this is set to argv[0] from main(), useful for error messages
	char *argv0;

	// this is a Scope object
	// some builtins are also available here (e.g. String), others (e.g. AstNode) are not
	struct Object *builtinscope;

	// this must be set when something fails and returns an error marker (e.g. NULL or STATUS_ERROR)
	// functions named blahblah_noerr() don't set this
	struct Object *err;

	struct AllObjects allobjects;

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
int interpreter_addbuiltin(struct Interpreter *interp, char *name, struct Object *val);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *interpreter_getbuiltin(struct Interpreter *interp, char *name);

#endif   // INTERPRETER_H
