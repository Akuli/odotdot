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
#include "string.h"

static void error_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	if (obj->data)
		cb((struct Object *) obj->data, data);
}

struct Object *errorobject_createclass_noerr(struct Interpreter *interp)
{
	// Error objects can have any attributes
	// TODO: use a message attribute instead of ->data?
	return classobject_new_noerr(interp, "Error", interp->builtins.Object, error_foreachref, true);
}

static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (err->data) {
		errorobject_throwfmt(interp, "AssertError", "setup was called twice");
		return NULL;
	}

	err->data = ARRAYOBJECT_GET(args, 1);
	OBJECT_INCREF(interp, ARRAYOBJECT_GET(args, 1));
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

	struct Object *msg = err->data;
	OBJECT_INCREF(interp, msg);
	return msg;
}

static struct Object *message_setter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, interp->builtins.String, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;
	struct Object *msg = ARRAYOBJECT_GET(args, 1);

	OBJECT_DECREF(interp, (struct Object*) err->data);
	err->data = msg;
	OBJECT_INCREF(interp, msg);
	return nullobject_get(interp);
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Error, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *err = ARRAYOBJECT_GET(args, 0);
	if (!check_data(interp, err)) return NULL;

	struct ClassObjectData *classdata = err->klass->data;
	return stringobject_newfromfmt(interp, "<%U: %D>", classdata->name, (struct Object*)err->data);
}

bool errorobject_addmethods(struct Interpreter *interp)
{
	if (!attribute_add(interp, interp->builtins.Error, "message", message_getter, message_setter)) return false;
	if (!method_add(interp, interp->builtins.Error, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Error, "to_debug_string", to_debug_string)) return false;
	return true;
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

	struct Object *err = object_new_noerr(interp, klass, str, NULL);
	OBJECT_DECREF(interp, klass);
	return err;    // may be NULL
}


void errorobject_throw(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);
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

	// builtins.ö must not define any errors that can't be constructed like this
	struct Object *err = classobject_newinstance(interp, klass, msg, NULL);
	OBJECT_DECREF(interp, klass);
	// don't decref msg, instead let err hold a reference to it
	if (!err) {
		OBJECT_DECREF(interp, msg);
		return;
	}

	errorobject_throw(interp, err);
	OBJECT_DECREF(interp, err);
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

void errorobject_printsimple(struct Interpreter *interp, struct Object *err)
{
	assert(!interp->err);

	if (err == interp->builtins.nomemerr) {
		// no more memory can be allocated
		fprintf(stderr, "MemError: not enough memory\n");
		return;
	}

	if (!print_ustr(interp, ((struct ClassObjectData *) err->klass->data)->name))
		goto cantprint;
	fputs(": ", stderr);
	if (!print_ustr( interp, *((struct UnicodeString *) err->data) ))
		goto cantprint;
	fputc('\n', stderr);
	return;

cantprint:
	fprintf(stderr, "\n%s: %s while printing an error\n",
		interp->err==interp->builtins.nomemerr ? "ran out of memory" : "another error occurred");
	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;
}
