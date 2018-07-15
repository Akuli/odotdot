#include "errors.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "../utf8.h"
#include "array.h"
#include "classobject.h"
#include "stackframe.h"
#include "string.h"

// stack is kind of funny
// it can't be set to an array when running in an asd_noerr function because arrayobject_newempty() needs error objects
// so if it's NULL, it should be set to an empty array whenever it's used
struct ErrorData {
	struct Object *message;    // String
	struct Object *stack;      // Array of StackFrames or NULL, can be empty
};


static void error_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	struct ErrorData edata = *(struct ErrorData*) data;
	cb(edata.message, cbdata);
	if (edata.stack)
		cb(edata.stack, cbdata);
}

static void error_destructor(void *data)
{
	free(data);
}

static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	struct ErrorData *data = malloc(sizeof(struct ErrorData));
	if (!data) {
		// can't recurse because nomemerr is created very early, and only 'new Error' runs this
		errorobject_thrownomem(interp);
		return NULL;
	}

	// this must be subclassing-friendly, that's why stuff is set to dummy values here and later changed in setup
	// this is not documented, but it doesn't really matter
	// the important thing is that forgetting to call super setup in the subclass doesn't cause a segfault
	// see also setup() below
	data->message = interp->strings.empty;
	OBJECT_INCREF(interp, data->message);
	data->stack = NULL;

	struct Object *err = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=data, .foreachref=error_foreachref, .destructor=error_destructor});
	if (!err) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->message);
		return NULL;
	}
	return err;
}

struct Object *errorobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, interp->builtins.Object, newinstance);
}


static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return false;

	struct ErrorData *data = ((struct Object*) thisdata.data)->objdata.data;
	assert(data);

	OBJECT_DECREF(interp, data->message);
	data->message = ARRAYOBJECT_GET(args, 0);
	OBJECT_INCREF(interp, data->message);
	return true;
}


ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Error, ErrorData, message)

// TODO: use an empty array instead of none?
static struct Object *stack_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ErrorData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	if (!data->stack) {
		if (!(data->stack = arrayobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->stack);
	return data->stack;
}

static bool message_setter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct ErrorData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_DECREF(interp, data->message);
	data->message = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, data->message);
	return true;
}

static bool print_ustr(struct Interpreter *interp, struct UnicodeString u)
{
	char *utf8;
	size_t utf8len;
	if (!utf8_encode(interp, u, &utf8, &utf8len))
		return false;

	for (size_t i = 0; i < utf8len; i++)
		fputc(utf8[i], stderr);
	free(utf8);
	return true;
}

// builtins.ö replaces this stupid thing with a method that actually prints the stack
static bool print_stack(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;
	struct Object *err = thisdata.data;

	if (err == interp->builtins.nomemerr) {
		// no more memory can be allocated
		fprintf(stderr, "MemError: not enough memory\n");
		return true;
	}

	if (!print_ustr(interp, ((struct ClassObjectData *) err->klass->objdata.data)->name))
		return false;
	fputs(": ", stderr);
	struct ErrorData *data = err->objdata.data;
	if (!print_ustr( interp, *((struct UnicodeString *) data->message->objdata.data) ))
		return false;
	fputc('\n', stderr);
	return true;
}

bool errorobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Error, "message", message_getter, message_setter)) return false;
	if (!attribute_add(interp, interp->builtins.Error, "stack", stack_getter, NULL)) return false;
	if (!method_add_noret(interp, interp->builtins.Error, "setup", setup)) return false;
	if (!method_add_noret(interp, interp->builtins.Error, "print_stack", print_stack)) return false;
	return true;
}


struct Object *errorobject_createmarkererrorclass(struct Interpreter *interp)
{
	assert(interp->builtins.Error);
	return classobject_new(interp, "MarkerError", interp->builtins.Error, newinstance);
}


struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp)
{
#define MESSAGE "not enough memory"
	struct UnicodeString u;
	u.len = sizeof(MESSAGE)-1;
	u.val = malloc(sizeof(unicode_char) * u.len);
	if (!u.val)
		return NULL;

	// can't memcpy because different types
	for (unsigned int i=0; i < u.len; i++)
		u.val[i] = MESSAGE[i];

	struct Object *str = stringobject_newfromustr_noerr(interp, u);
	if (!str)
		return NULL;

	// the MemError class is not stored anywhere else, builtins_setup() looks it up from interp->builtins.nomemerr
	struct Object *klass = classobject_new_noerr(interp, interp->builtins.Error, newinstance);
	if (!klass) {
		OBJECT_DECREF(interp, str);   // takes care of ustr and ustr->val
		return NULL;
	}

	struct ErrorData *data = malloc(sizeof(struct ErrorData));
	if (!data) {
		OBJECT_DECREF(interp, klass);
		OBJECT_DECREF(interp, str);
		return NULL;
	}

	data->message = str;
	data->stack = NULL;

	struct Object *err = object_new_noerr(interp, klass, (struct ObjectData){.data=data, .foreachref=error_foreachref, .destructor=error_destructor});
	OBJECT_DECREF(interp, klass);
	if (!err) {
		OBJECT_DECREF(interp, data->message);
		free(data);
		return NULL;
	}
	return err;
}


struct Object *errorobject_new(struct Interpreter *interp, struct Object *errorclass, struct Object *messagestring)
{
	struct ErrorData *data = malloc(sizeof(struct ErrorData));
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	data->message = messagestring;
	OBJECT_INCREF(interp, messagestring);
	data->stack = NULL;

	// builtins.ö must not define any errors that can't be constructed like this
	struct Object *err = object_new_noerr(interp, errorclass, (struct ObjectData){.data=data, .foreachref=error_foreachref, .destructor=error_destructor});
	if (!err) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, data->message);
		free(data);
		return NULL;
	}
	return err;
}


void errorobject_throw(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);

	struct ErrorData *data = err->objdata.data;
	if (!data->stack) {
		if (!(data->stack = arrayobject_newempty(interp)))
			return;
	}
	if (ARRAYOBJECT_LEN(data->stack) == 0) {
		// the stack hasn't been set yet, so we're throwing this error for the 1st time
		if (!stackframeobject_getstack(interp, data->stack))
			return;
	}

	interp->err = err;
	OBJECT_INCREF(interp, err);
}

void errorobject_throwfmt(struct Interpreter *interp, char *classname, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	struct Object *msg = stringobject_newfromvfmt(interp, fmt, ap);
	va_end(ap);
	if (!msg)
		return;

	struct Object *klass = interpreter_getbuiltin(interp, classname);
	if (!klass) {
		OBJECT_DECREF(interp, msg);
		return;
	}

	struct Object *err = errorobject_new(interp, klass, msg);
	OBJECT_DECREF(interp, klass);
	OBJECT_DECREF(interp, msg);
	if (!err)
		return;

	errorobject_throw(interp, err);
	OBJECT_DECREF(interp, err);
}


void errorobject_print(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);

	if (!method_call_noret(interp, err, "print_stack", NULL)) {
		fprintf(stderr, "\n%s: %s while printing an error\n", interp->argv0, interp->err==interp->builtins.nomemerr?"ran out of memory":"another error occurred");
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;
	}
}
