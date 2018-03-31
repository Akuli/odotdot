#ifndef CONTEXT_H
#define CONTEXT_H

struct Interpreter;    // defined in interpreter.h
struct Context {
	struct Context *parentctx;
	struct Interpreter *interp;
	struct HashTable *localvars;    // keys are struct UnicodeString*, values are struct Object*
};

// returns NULL on no mem because interp doesn't have ->nomemerr yet
struct Context *context_newglobal(struct Interpreter *interp);

struct Context *context_newsub(struct Context *parentctx);

void context_free(struct Context *ctx);

#endif   // CONTEXT_H
