#include "context.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "common.h"
#include "hashtable.h"
#include "interpreter.h"    // IWYU pragma: keep
#include "objectsystem.h"
#include "unicode.h"
#include "objects/errors.h"

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
		errorobject_setnomem(parentctx->interp, errptr);
		return NULL;
	}
	ctx->parentctx = parentctx;
	return ctx;
}

int context_setlocalvar(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value)
{
	struct UnicodeString *nameptr = unicodestring_copy(name);
	if (!nameptr) {
		errorobject_setnomem(ctx->interp, errptr);
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
		errorobject_setnomem(ctx->interp, errptr);
		return STATUS_ERROR;
	}
	assert(status == STATUS_OK);
	OBJECT_INCREF(ctx->interp, value);
	return STATUS_OK;
}

int context_setvarwheredefined(struct Context *ctx, struct Object **errptr, struct UnicodeString name, struct Object *value)
{
	// this is a do-while to make sure it fails with ctx == NULL
	do {
		// check if the var is defined in this context
		void *junk;
		if (hashtable_get(ctx->localvars, &name, unicodestring_hash(name), &junk, NULL))
			return context_setlocalvar(ctx, errptr, name, value);
	} while ((ctx = ctx->parentctx));    // no luck, look up from parent context

	errorobject_setwithfmt(ctx->interp, errptr, "there's no '%U' variable", name);
	return STATUS_ERROR;
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
	if (!res) {
		errorobject_setwithfmt(ctx->interp, errptr, "there's no '%U' variable", name);   // allocates mem
		return NULL;
	}
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
