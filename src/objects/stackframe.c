#include "stackframe.h"
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../stack.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "string.h"

ATTRIBUTE_DEFINE_SIMPLE_GETTER(filename, StackFrame)
ATTRIBUTE_DEFINE_SIMPLE_GETTER(lineno, StackFrame)

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	errorobject_throwfmt(interp, "TypeError", "cannot create new StackFrame objects");
	return NULL;
}

struct Object *stackframeobject_createclass(struct Interpreter *interp)
{
	struct Object *klass = classobject_new(interp, "StackFrame", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!attribute_add(interp, klass, "filename", filename_getter, NULL)) goto error;
	if (!attribute_add(interp, klass, "lineno", lineno_getter, NULL)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


static struct Object *new_frame_object(struct Interpreter *interp, struct StackFrame f)
{
	struct Object *fobj = object_new_noerr(interp, interp->builtins.StackFrame, NULL, NULL, NULL);
	if (!fobj) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	// TODO: is utf8 always the best possible file system encoding?
	struct UnicodeString u;
	if (!utf8_decode(interp, f.filename, strlen(f.filename), &u))
		goto error;

	struct Object *filename = stringobject_newfromustr(interp, u);
	free(u.val);
	if (!filename)
		goto error;

	bool ok = attribute_settoattrdata(interp, fobj, "filename", filename);
	OBJECT_DECREF(interp, filename);
	if (!ok)
		goto error;

	struct Object *lineno = integerobject_newfromlonglong(interp, f.lineno);
	if (!lineno)
		goto error;

	ok = attribute_settoattrdata(interp, fobj, "lineno", lineno);
	OBJECT_DECREF(interp, lineno);
	if (!ok)
		goto error;

	return fobj;

error:
	OBJECT_DECREF(interp, fobj);
	return NULL;
}

struct Object *stackframeobject_getstack(struct Interpreter *interp)
{
	struct Object *result = arrayobject_newempty(interp);
	if (!result)
		return NULL;

	for (struct StackFrame *f = interp->stack; f != interp->stackptr; f++) {
		struct Object *fobj = new_frame_object(interp, *f);
		if (!fobj) {
			OBJECT_DECREF(interp, result);
			return NULL;
		}

		bool ok = arrayobject_push(interp, result, fobj);
		OBJECT_DECREF(interp, fobj);
		if (!ok) {
			OBJECT_DECREF(interp, result);
			return NULL;
		}
	}

	return result;
}
