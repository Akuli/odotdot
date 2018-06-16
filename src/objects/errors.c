#include "errors.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "classobject.h"
#include "mapping.h"
#include "null.h"
#include "stackframe.h"
#include "string.h"

struct ErrorData {
	struct Object *message;    // String
	struct Object *stack;      // Array of StackFrames, or the null object (i.e. never NULL)
};


static void error_foreachref(struct Object *err, void *cbdata, classobject_foreachrefcb cb)
{
	struct ErrorData *data = err->data;
	if (data) {
		cb(data->message, cbdata);
		cb(data->stack, cbdata);
	}
}

static void error_destructor(struct Object *err)
{
	if (err->data)
		free(err->data);
}

struct Object *errorobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, "Error", interp->builtins.Object, error_foreachref, true);
}


static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	struct Object *msg = ARRAYOBJECT_GET(args, 1);

	if (err->data) {
		errorobject_throwfmt(interp, "AssertError", "setup was called twice");
		return NULL;
	}

	struct ErrorData *data = malloc(sizeof(struct ErrorData));
	if (!data) {
		// can't recurse because nomemerr is created very early, and only 'new Error' runs this
		errorobject_thrownomem(interp);
		return NULL;
	}
	data->message = msg;
	OBJECT_INCREF(interp, msg);
	data->stack = interp->builtins.null;
	OBJECT_INCREF(interp, interp->builtins.null);

	err->data = data;
	err->destructor = error_destructor;
	return nullobject_get(interp);
}


// Error is subclassable, so it's possible to define a subclass of Error that overrides setup without calling Error's setup
static bool check_data(struct Interpreter *interp, struct Object *err)
{
	if (!err->data) {
		errorobject_throwfmt(interp, "AssertError", "Error's setup method wasn't called");
		return false;
	}
	return true;
}


static struct Object *message_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	struct ErrorData *data = err->data;
	OBJECT_INCREF(interp, data->message);
	return data->message;
}

static struct Object *message_setter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	struct ErrorData *data = err->data;
	OBJECT_DECREF(interp, data->message);
	data->message = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, data->message);
	return nullobject_get(interp);
}

static struct Object *stack_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	struct ErrorData *data = err->data;
	OBJECT_INCREF(interp, data->stack);
	return data->stack;
}

static struct Object *stack_setter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	// doesn't make sense to check if the new stack contains nothing but StackFrames
	// because it's possible to push anything to the array
	// the Object must be null or an Array, that is checked below
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.Object, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;
	struct Object *newstack = ARRAYOBJECT_GET(args, 1);

	if (newstack != interp->builtins.null && !classobject_isinstanceof(newstack, interp->builtins.Array)) {
		errorobject_throwfmt(interp, "TypeError", "expected null or an Array, got %D", newstack);
		return NULL;
	}

	struct ErrorData *data = err->data;
	OBJECT_DECREF(interp, data->stack);
	data->stack = newstack;
	OBJECT_INCREF(interp, newstack);
	return nullobject_get(interp);
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
static struct Object *print_stack(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	if (err == interp->builtins.nomemerr) {
		// no more memory can be allocated
		fprintf(stderr, "MemError: not enough memory\n");
		return nullobject_get(interp);
	}

	if (!print_ustr(interp, ((struct ClassObjectData *) err->klass->data)->name))
		return NULL;
	fputs(": ", stderr);
	struct ErrorData *data = err->data;
	if (!print_ustr( interp, *((struct UnicodeString *) data->message->data) ))
		return NULL;
	fputc('\n', stderr);

	return nullobject_get(interp);
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	struct ClassObjectData *classdata = err->klass->data;
	struct ErrorData *errdata = err->data;
	return stringobject_newfromfmt(interp, "<%U: %D>", classdata->name, errdata->message);
}

bool errorobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Error, "message", message_getter, message_setter)) return false;
	if (!attribute_add(interp, interp->builtins.Error, "stack", stack_getter, stack_setter)) return false;
	if (!method_add(interp, interp->builtins.Error, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Error, "print_stack", print_stack)) return false;
	if (!method_add(interp, interp->builtins.Error, "to_debug_string", to_debug_string)) return false;
	return true;
}


// MarkerError objects have a value attribute, so 'return value;' can set the value attribute and throw
ATTRIBUTE_DEFINE_SIMPLE_GETTER(value, MarkerError)
ATTRIBUTE_DEFINE_SIMPLE_SETTER(value, MarkerError, Object)

struct Object *errorobject_createmarkererrorclass(struct Interpreter *interp)
{
	assert(interp->builtins.Error);

	struct Object *klass = classobject_new(interp, "MarkerError", interp->builtins.Error, NULL, false);
	if (!klass)
		return NULL;

	if (!attribute_add(interp, klass, "value", value_getter, value_setter)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}


// TODO: stop copy/pasting this from string.c and actually fix things
static void string_destructor(struct Object *str)
{
	struct UnicodeString *data = str->data;
	free(data->val);
	free(data);
}

struct Object *errorobject_createnomemerr_noerr(struct Interpreter *interp)
{
	// message string is created here because string constructors use interp->builtins.nomemerr and interp->err
	// string objects are simple, the data is just a UnicodeString pointer
	struct UnicodeString *ustr = malloc(sizeof(struct UnicodeString));
	if (!ustr)
		return NULL;

#define MESSAGE "not enough memory"
	ustr->len = sizeof(MESSAGE) - 1;
	ustr->val = malloc(sizeof(unicode_char) * ustr->len);
	if (!(ustr->val)) {
		free(ustr);
		return NULL;
	}

	// can't use memcpy because different types
	for (size_t i=0; i < ustr->len; i++)
		ustr->val[i] = MESSAGE[i];
#undef MESSAGE

	struct Object *str = object_new_noerr(interp, interp->builtins.String, ustr, string_destructor);
	if (!str) {
		free(ustr->val);
		free(ustr);
		return NULL;
	}

	// the MemError class is not stored anywhere else, builtins_setup() looks it up from interp->builtins.nomemerr
	struct Object *klass = classobject_new_noerr(interp, "MemError", interp->builtins.Error, NULL, false);
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

	assert(interp->builtins.null);
	data->message = str;
	data->stack = interp->builtins.null;
	OBJECT_INCREF(interp, interp->builtins.null);

	struct Object *err = object_new_noerr(interp, klass, data, error_destructor);
	OBJECT_DECREF(interp, klass);
	if (!err) {
		OBJECT_DECREF(interp, data->message);
		OBJECT_DECREF(interp, data->stack);
		free(data);
		return NULL;
	}
	return err;
}


void errorobject_throw(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);

	// check_data can throw, but it shouldn't recurse unless something's badly broken ....
	if (!check_data(interp, err))
		return;

	struct ErrorData *data = err->data;
	if (data->stack == interp->builtins.null) {
		// the stack hasn't been set yet, so we're throwing this error for the 1st time
		struct Object *stack = stackframeobject_getstack(interp);
		if (!stack)
			return;
		OBJECT_DECREF(interp, data->stack);
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

	struct ErrorData *data = malloc(sizeof(struct ErrorData));
	if (!data) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, klass);
		OBJECT_DECREF(interp, msg);
		return;
	}

	data->message = msg;
	data->stack = interp->builtins.null;
	OBJECT_INCREF(interp, interp->builtins.null);

	// builtins.ö must not define any errors that can't be constructed like this
	struct Object *err = classobject_newinstance(interp, klass, data, error_destructor);
	OBJECT_DECREF(interp, klass);
	if (!err) {
		OBJECT_DECREF(interp, data->message);
		OBJECT_DECREF(interp, data->stack);
		free(data);
		return;
	}

	errorobject_throw(interp, err);
	OBJECT_DECREF(interp, err);
}


void errorobject_print(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);

	// the stack printing is implemented in pure Ö
	struct Object *res = method_call(interp, err, "print_stack", NULL);
	if (res) {
		OBJECT_DECREF(interp, res);
	} else {
		fprintf(stderr, "\n%s: %s while printing an error\n", interp->argv0, interp->err==interp->builtins.nomemerr?"ran out of memory":"another error occurred");
		OBJECT_DECREF(interp, interp->err);
		interp->err = NULL;
	}
}
