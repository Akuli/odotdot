#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "allobjects.h"

// these are defined in other files that need to include this file
// stupid IWYU doesn't get this.....
struct Context;
struct Object;

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
	struct {
		struct Object *Array;
		struct Object *AstNode;
		struct Object *Block;
		struct Object *Class;    // the class of class objects
		struct Object *Error;
		struct Object *Function;
		struct Object *Integer;
		struct Object *Mapping;
		struct Object *Object;
		struct Object *Scope;
		struct Object *String;

		struct Object *nomemerr;
	} builtins;
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
