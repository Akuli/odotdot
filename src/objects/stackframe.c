#include "stackframe.h"
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../objectsystem.h"
#include "../stack.h"
#include "../utf8.h"
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"
#include "string.h"

struct StackFrameData {
	struct Object *filename;
	struct Object *lineno;
	struct Object *scope;
};

// sf = stack frame
static void sf_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct StackFrameData sfdata = *(struct StackFrameData*) data;
	cb(sfdata.filename, cbdata);
	cb(sfdata.lineno, cbdata);
	cb(sfdata.scope, cbdata);
}

static void sf_destructor(void *data) { free(data); }

static struct Object *filename_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.StackFrame, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct StackFrameData sfdata = *(struct StackFrameData*) ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_INCREF(interp, sfdata.filename);
	return sfdata.filename;
}

static struct Object *lineno_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.StackFrame, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct StackFrameData sfdata = *(struct StackFrameData*) ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_INCREF(interp, sfdata.lineno);
	return sfdata.lineno;
}

static struct Object *scope_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.StackFrame, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct StackFrameData sfdata = *(struct StackFrameData*) ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_INCREF(interp, sfdata.scope);
	return sfdata.scope;
}

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
	if (!attribute_add(interp, klass, "scope", scope_getter, NULL)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


static struct Object *new_frame_object(struct Interpreter *interp, struct StackFrame f)
{
	// TODO: is utf8 always the best possible file system encoding?
	struct UnicodeString u;
	if (!utf8_decode(interp, f.filename, strlen(f.filename), &u))
		return NULL;

	struct Object *filename = stringobject_newfromustr(interp, u);
	if (!filename)
		return NULL;

	struct Object *lineno = integerobject_newfromlonglong(interp, f.lineno);
	if (!lineno) {
		OBJECT_DECREF(interp, filename);
		return NULL;
	}

	struct StackFrameData *sfdata = malloc(sizeof *sfdata);
	if (!sfdata) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, lineno);
		OBJECT_DECREF(interp, filename);
		return NULL;
	}
	sfdata->filename = filename;
	sfdata->lineno = lineno;
	sfdata->scope = f.scope;
	OBJECT_INCREF(interp, f.scope);

	struct Object *res = object_new_noerr(interp, interp->builtins.StackFrame, (struct ObjectData){.data=sfdata, .foreachref=sf_foreachref, .destructor=sf_destructor});
	if (!res) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, sfdata->filename);
		OBJECT_DECREF(interp, sfdata->lineno);
		OBJECT_DECREF(interp, sfdata->scope);
		free(sfdata);
		return NULL;
	}
	return res;
}

struct Object *stackframeobject_getstack(struct Interpreter *interp)
{
	struct Object *result = arrayobject_newwithcapacity(interp, interp->stackptr - interp->stack);
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
