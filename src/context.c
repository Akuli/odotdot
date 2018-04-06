#include "context.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>    // for printing messages about refcount problems
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hashtable.h"
#include "interpreter.h"
#include "unicode.h"

// grep for this function's name, it's copy/pasted in a couple other places too
static int compare_unicode_strings(void *a, void *b, void *userdata)
{
	assert(!userdata);
	struct UnicodeString *astr=a, *bstr=b;
	if (astr->len != bstr->len)
		return 0;

	// memcmp is not reliable :( https://stackoverflow.com/a/11995514
	for (size_t i=0; i < astr->len; i++) {
		if (astr->val[i] != bstr->val[i])
			return 0;
	}
	return 1;
}


struct Context *context_newglobal(struct Interpreter *interp)
{
	struct Context *ctx = malloc(sizeof(struct Context));
	if (!ctx)
		return NULL;

	ctx->parentctx = NULL;
	ctx->interp = interp;
	ctx->localvars = hashtable_new(compare_unicode_strings);
	if (!ctx->localvars) {
		free(ctx);
		return NULL;
	}
	return ctx;
}

struct Context *context_newsub(struct Context *parentctx, struct Object **errptr)
{
	struct Context *ctx = context_newglobal(parentctx->interp);
	if (!ctx) {
		*errptr = parentctx->interp->nomemerr;
		return NULL;
	}
	ctx->parentctx = parentctx;
	return ctx;
}

int context_setlocalvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value)
{
	struct UnicodeString *nameptr = unicodestring_copy(name);
	if (!nameptr) {
		*errptr = ctx->interp->nomemerr;
		return STATUS_ERROR;
	}

	// if it's there already, be sure to free the name pointer and decref the old value correctly
	struct Object *oldval;
	if (hashtable_get(ctx->localvars, nameptr, unicodestring_hash(name), (void **) (&oldval), NULL)) {
		OBJECT_DECREF(ctx->interp, oldval);
		// FIXME: hmm..... how to free the old name key??
		assert(0);
	}

	int status = hashtable_set(ctx->localvars, nameptr, unicodestring_hash(name), value, NULL);
	if (status == STATUS_NOMEM) {
		*errptr = ctx->interp->nomemerr;
		return STATUS_ERROR;
	}
	assert(status == STATUS_OK);
	OBJECT_INCREF(ctx->interp, value);
	return STATUS_OK;
}

int context_setvarwheredefined(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value)
{
	while (ctx) {
		// check if the var is defined in this context
		void *junk;
		if (hashtable_get(ctx->localvars, &name, unicodestring_hash(name), &junk, NULL)) {
			// yes
			return context_setlocalvar(ctx, errptr, name, value);
		}
		// no luck... look up from parent context
		ctx = ctx->parentctx;
	}

	// this is my attempt at creating a concatenated UnicodeString
	char prefixchararr[] = "no variable named '";
	struct UnicodeString msg;
	// sizeof(prefixchararr)-1 == strlen(prefixchararr)
	// last +1 is for trailing '
	msg.len = sizeof(prefixchararr)-1 + name.len + 1;
	msg.val = malloc(sizeof(uint32_t) * msg.len);
	if (!msg.val) {
		*errptr = ctx->interp->nomemerr;
		return STATUS_ERROR;
	}

	for (int i=0; i < (int)(sizeof(prefixchararr)-1); i++)
		msg.val[i] = prefixchararr[i];   // assumes ASCII
	memcpy(msg.val + (sizeof(prefixchararr)-1), name.val, sizeof(uint32_t) * name.len);
	msg.val[msg.len-1] = '\'';

	// now we have a carefully created msg, almost done
	// TODO: errorobject_newfromstringobject should take something like interp or ctx
	// as an argument instead of the Error classinfo
	assert(0);
	//struct Object *err = errorobject_newfromstringobject(????)

}

struct Object *context_getvar_nomalloc(struct Context *ctx, struct UnicodeString name)
{
	while (ctx) {
		struct Object *val;
		if (hashtable_get(ctx->localvars, &name, unicodestring_hash(name), (void **)(&val), NULL)) {
			OBJECT_INCREF(ctx->interp, val);
			return val;
		}
		ctx = ctx->parentctx;
	}
	return NULL;
}

struct Object *context_getvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name)
{
	struct Object *res = context_getvar_nomalloc(ctx, name);   // returns a new reference
	assert(res);    // i didn't feel like copy/pasting the incomplete mess from context_setvarwheredefined()
	return res;
}

static void clear_item(void *ustrkey, void *valobj, void *interp)
{
	free(((struct UnicodeString *) ustrkey)->val);
	free(ustrkey);
	OBJECT_DECREF(interp, valobj);
}

void context_free(struct Context *ctx)
{
	hashtable_fclear(ctx->localvars, clear_item, ctx->interp);
	hashtable_free(ctx->localvars);
	free(ctx);
}
