#include "context.h"
#include <assert.h>
#include <stdlib.h>
#include "hashtable.h"
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

struct Context *context_newsub(struct Context *parentctx)
{
	struct Context *ctx = context_newglobal(parentctx->interp);
	if (!ctx)
		return NULL;
	ctx->parentctx = parentctx;
	return ctx;
}

static void clear_item(void *ustrkey, void *valobj, void *junkdata)
{
	free(((struct UnicodeString *) ustrkey)->val);
	free(ustrkey);
	// TODO: do something with val?
}

void context_free(struct Context *ctx)
{
	hashtable_fclear(ctx->localvars, clear_item, NULL);
	hashtable_free(ctx->localvars);
	free(ctx);
}
