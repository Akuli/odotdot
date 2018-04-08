#include "object.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"


struct ObjectClassInfo *objectobject_createclass(void)
{
	return objectclassinfo_new("Object", NULL, NULL, NULL);
}


static inline int is_a_wovel(char c)
{
	// TODO: how about e.g. ä, ö? they are wovels but not bytes in utf8
#define f(x) c==(x)
	return f('A')||f('a')||
		f('E')||f('e')||
		f('I')||f('i')||
		f('O')||f('o')||
		f('U')||f('u')||
		f('Y')||f('y');
#undef f
}

static struct Object *to_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	assert(nargs == 1);     // TODO: better type check

	// TODO: utf8 strings are not really a good solution here....
	char res[100];
	char *name = ((struct ObjectClassInfo*) args[0]->klass->data)->name;
	sprintf(res, "<%s %.50s at %p>", is_a_wovel(name[0]) ? "an" : "a", name, (void*) args[0]);
	return stringobject_newfromcharptr(ctx->interp, errptr, res);
}

static struct Object *to_debug_string(struct Context *ctx, struct Object **errptr, struct Object **args, size_t nargs)
{
	assert(nargs == 1);
	return method_call(ctx, errptr, args[0], "to_string", NULL);
}

int objectobject_addmethods(struct Interpreter *interp, struct Object **errptr)
{
	struct Object *objectclass = interpreter_getbuiltin(interp, errptr, "Object");
	if (!objectclass)
		return STATUS_ERROR;

	if (method_add(interp, errptr, objectclass, "to_string", to_string) == STATUS_ERROR) goto error;
	if (method_add(interp, errptr, objectclass, "to_debug_string", to_debug_string) == STATUS_ERROR) goto error;

	OBJECT_DECREF(interp, objectclass);
	return STATUS_OK;

error:
	OBJECT_DECREF(interp, objectclass);
	return STATUS_ERROR;
}
