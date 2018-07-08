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
#include "option.h"
#include "stackframe.h"
#include "string.h"

struct ErrorData {
	struct Object *message;    // String
	struct Object *stack;      // Array of StackFrames, or the null object (i.e. never NULL)
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
	data->message = stringobject_newfromcharptr(interp, "");
	if (!data->message)
		return NULL;
	data->stack = NULL;

	struct Object *err = object_new_noerr(interp, ARRAYOBJECT_GET(args, 0), (struct ObjectData){.data=data, .foreachref=error_foreachref, .destructor=error_destructor});
	if (!err) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	return err;
}

struct Object *errorobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, interp->builtins.Object, newinstance);
}


static struct Object *setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, NULL)) return NULL;

	struct ErrorData *data = ((struct Object*) thisdata.data)->objdata.data;
	assert(data);

	OBJECT_DECREF(interp, data->message);
	data->message = ARRAYOBJECT_GET(args, 0);
	OBJECT_INCREF(interp, data->message);
	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}


ATTRIBUTE_DEFINE_STRUCTDATA_GETTER(Error, ErrorData, message)

static struct Object *stack_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ErrorData data = *(struct ErrorData*) ARRAYOBJECT_GET(args, 0)->objdata.data;
	if (data.stack)
		return optionobject_new(interp, data.stack);
	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}

static struct Object *message_setter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ErrorData *data = ARRAYOBJECT_GET(args, 0)->objdata.data;
	OBJECT_DECREF(interp, data->message);
	data->message = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, data->message);
	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}

static struct Object *stack_setter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	// doesn't make sense to check if the new stack contains nothing but StackFrames
	// because it's possible to push anything to the array
	// the Object must be null or an Array, that is checked below
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.Option, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	struct ErrorData *data = err->objdata.data;
	struct Object *newstackopt = ARRAYOBJECT_GET(args, 1);

	if (newstackopt == interp->builtins.none) {
		if (data->stack)
			OBJECT_DECREF(interp, data->stack);
		data->stack = NULL;
		OBJECT_INCREF(interp, interp->builtins.none);
		return interp->builtins.none;
	}

	struct Object *newstack = OPTIONOBJECT_VALUE(newstackopt);
	if (!check_type(interp, interp->builtins.Array, newstack))
		return NULL;

	if (data->stack)
		OBJECT_DECREF(interp, data->stack);
	data->stack = newstack;
	OBJECT_INCREF(interp, newstack);

	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
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
static struct Object *print_stack(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = thisdata.data;

	if (err == interp->builtins.nomemerr) {
		// no more memory can be allocated
		fprintf(stderr, "MemError: not enough memory\n");
		OBJECT_INCREF(interp, interp->builtins.none);
		return interp->builtins.none;
	}

	if (!print_ustr(interp, ((struct ClassObjectData *) err->klass->objdata.data)->name))
		return NULL;
	fputs(": ", stderr);
	struct ErrorData *data = err->objdata.data;
	if (!print_ustr( interp, *((struct UnicodeString *) data->message->objdata.data) ))
		return NULL;
	fputc('\n', stderr);

	OBJECT_INCREF(interp, interp->builtins.none);
	return interp->builtins.none;
}

bool errorobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Error, "message", message_getter, message_setter)) return false;
	if (!attribute_add(interp, interp->builtins.Error, "stack", stack_getter, stack_setter)) return false;
	if (!method_add(interp, interp->builtins.Error, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Error, "print_stack", print_stack)) return false;
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
		// the stack hasn't been set yet, so we're throwing this error for the 1st time
		struct Object *stack = stackframeobject_getstack(interp);
		if (!stack)
			return;
		data->stack = stack;
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

	struct Object *res = method_call(interp, err, "print_stack", NULL);
	if (res) {
		OBJECT_DECREF(interp, res);
	} else {
		fprintf(stderr, "\n%s: %s while printing an error\n", interp->argv0, interp->err==interp->builtins.nomemerr?"ran out of memory":"another error occurred");
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;
	}
}
