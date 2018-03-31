#ifndef CONTEXT_H
#define CONTEXT_H

#include "objectsystem.h"

struct Interpreter;    // defined in interpreter.h
struct Context {
	struct Context *parentctx;
	struct Interpreter *interp;
	struct HashTable *localvars;    // keys are struct UnicodeString*, values are struct Object*
};

// returns NULL on no mem
// doesn't take an errptr because interp->nomemerr is NULL when this is called
struct Context *context_newglobal(struct Interpreter *interp);

// assumes that parentctx->interp->nomemerr is set
struct Context *context_newsub(struct Context *parentctx, struct Object **errptr);

// these return STATUS_OK or STATUS_ERROR
int context_setlocalvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value);
int context_setvarwheredefined(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value);

// returns NULL on error
struct Object *context_getvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name);

void context_free(struct Context *ctx);

#endif   // CONTEXT_H
