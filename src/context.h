#ifndef CONTEXT_H
#define CONTEXT_H

#include "objectsystem.h"       // IWYU pragma: keep

// Interpreter defined in interpreter.h but it needs to include this file
// IWYU doesn't get this and there seems to be no way to pragma keep structs :(
struct Interpreter;
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

// these copy the name and return STATUS_OK or STATUS_ERROR
int context_setlocalvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value);
int context_setvarwheredefined(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value);

// returns NULL and sets errptr when the var is not found
struct Object *context_getvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name);

// returns NULL when the var is not found instead of taking an errptr
// never calls malloc(), cannot run out of mem
struct Object *context_getvar_nomalloc(struct Context *ctx, struct UnicodeString name);

void context_free(struct Context *ctx);

#endif   // CONTEXT_H
