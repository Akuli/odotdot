#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "allobjects.h"
#include "stack.h"

// these are defined in other files that need to include this file
// stupid IWYU doesn't get this.....
struct Object;
struct StackFrame;

struct Interpreter {
	// this is set to argv[0] from main(), useful for error messages
	char *argv0;

	// set by builtins_setup(), it calls
	// TODO: paths aren't char* on windows, so choose another type here
	char *stdpath;

	// this is a Scope object
	// some builtins are also available here (e.g. String), others (e.g. AstNode) are not
	struct Object *builtinscope;

	// this must be set when something fails and returns an error marker (e.g. NULL or false)
	// functions named blahblah_noerr() don't set this
	struct Object *err;

	struct AllObjects allobjects;

	// this holds references to built-in classes, functions and stuff
	struct {
		struct Object *ArbitraryAttribs;
		struct Object *Array;
		struct Object *AstNode;
		struct Object *Block;
		struct Object *Bool;
		struct Object *Class;    // the class of class objects
		struct Object *Error;
		struct Object *Function;
		struct Object *Integer;
		struct Object *Library;
		struct Object *Mapping;
		struct Object *MarkerError;   // an Error subclass for return, will probably be used for break and continue later
		struct Object *Object;   // yes, this works
		struct Object *Scope;
		struct Object *StackFrame;
		struct Object *String;

		struct Object *null;       // NOT the (void*)0 NULL, see Objects/null.{c,h}
		struct Object *yes, *no;   // Bool objects, avoid name clash with stdbool.h
		struct Object *nomemerr;
	} builtins;

	// String objects that are often needed, mostly for optimizing
	struct {
		struct Object *else_;
		struct Object *empty;
		struct Object *export;
		struct Object *return_;
	} strings;

	struct {
		struct Object *filelibcache;   // Mapping, keys are path strings, values are Library objects
		struct Object *importers;      // Array of importer functions, see docs
	} importstuff;

	// contains arrays of functions
	// (x + y) loops through oparrays.add and calls the functions until one of them returns non-null
	// see also operator.h
	struct {
		// the Operator enum in operator.h shows what these short names mean
		struct Object *add, *sub, *mul, *div, *eq, *lt;
	} oparrays;

	struct StackFrame stack[STACK_MAX];
	struct StackFrame *stackptr;   // pointer to above the top of the stack
};

// prints a message to stderr and returns NULL on error
// pretty much nothing is ready after calling this, use builtins_setup()
struct Interpreter *interpreter_new(char *argv0);

// never fails
void interpreter_free(struct Interpreter *interp);

// convenience methods, these assert that name is valid UTF-8

// returns false on error
bool interpreter_addbuiltin(struct Interpreter *interp, char *name, struct Object *val);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *interpreter_getbuiltin(struct Interpreter *interp, char *name);

#endif   // INTERPRETER_H
