#include "lambdabuiltin.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "attribute.h"
#include "check.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "unicode.h"

struct LambdaData {
	struct Object *argnames;   // names of all arguments as String objects
	struct Object *optnames;   // names of all options as String objects
	struct Object *argtypes;   // an array of interp->builtins.Object repeated ARRAYOBJECT_LEN(argnames) times, for check_args()
	struct Object *opttypes;   // a Mapping of each optname to interp->builtins.Object, for check_opts()
	struct Object *block;      // the block that the function was defined in
};

static void ldata_foreachref(void *ldata, object_foreachrefcb cb, void *cbdata)
{
	cb(((struct LambdaData*) ldata)->argnames, cbdata);
	cb(((struct LambdaData*) ldata)->optnames, cbdata);
	cb(((struct LambdaData*) ldata)->argtypes, cbdata);
	cb(((struct LambdaData*) ldata)->opttypes, cbdata);
	cb(((struct LambdaData*) ldata)->block, cbdata);
}

static void ldata_destructor(void *ldata)
{
	free(ldata);
}


static struct Object *runner(struct Interpreter *interp, struct ObjectData data, struct Object *args, struct Object *opts)
{
	struct LambdaData *ldata = data.data;
	if (!check_args_with_array(interp, args, ldata->argtypes)) return NULL;
	if (!check_opts_with_mapping(interp, opts, ldata->opttypes)) return NULL;

	struct Object *parentscope = attribute_get(interp, ldata->block, "definition_scope");
	if (!parentscope)
		return NULL;

	struct Object *scope = scopeobject_newsub(interp, parentscope);
	OBJECT_DECREF(interp, parentscope);
	if (!scope)
		return NULL;

	// add values of arguments...
	for (size_t i=0; i < ARRAYOBJECT_LEN(ldata->argnames); i++) {
		if (!mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), ARRAYOBJECT_GET(ldata->argnames, i), ARRAYOBJECT_GET(args, i))) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
	}

	// ...and options
	for (size_t i=0; i < ARRAYOBJECT_LEN(ldata->optnames); i++) {
		struct Object *val;
		int status = mappingobject_get(interp, opts, ARRAYOBJECT_GET(ldata->optnames, i), &val);
		if (status == -1) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
		if (status == 0)
			val = nullobject_get(interp);
		assert(val);

		bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), ARRAYOBJECT_GET(ldata->optnames, i), val);
		OBJECT_DECREF(interp, val);
		if (!ok) {
			OBJECT_DECREF(interp, scope);
			return NULL;
		}
	}

	struct Object *retval = blockobject_runwithreturn(interp, ldata->block, scope);
	OBJECT_DECREF(interp, scope);
	return retval;
}


static bool check_identifier(struct Interpreter *interp, struct UnicodeString u)
{
	if (u.len == 0)
		goto nope;
	if (!unicode_isidentifier1st(u.val[0]))
		goto nope;
	for (size_t i=1; i < u.len; i++) {
		if (!unicode_isidentifiernot1st(u.val[i]))
			goto nope;
	}
	return true;

nope:
	errorobject_throwfmt(interp, "ValueError", "\"%U\" is not a valid argument name", u);
	return false;
}

// parses a string like "argument1 argument2 some_option: another_option:"
// argnames and optnames should be array objects
// returns false and leaves contents of argnames and optnames to something random on failure
// bad things happen if string is not a String object
static bool parse_arg_and_opt_names(struct Interpreter *interp, struct Object *string, struct Object *argnames, struct Object *optnames)
{
	struct Object *splitted = stringobject_splitbywhitespace(interp, string);
	if (!splitted)
		return NULL;

	// arguments must come before options, this is set to true when the first option is found
	bool opts = false;
	for (size_t i=0; i < ARRAYOBJECT_LEN(splitted); i++) {
		struct UnicodeString u = *(struct UnicodeString *) ARRAYOBJECT_GET(splitted, i)->objdata.data;

		if (u.len == 0) {
			check_identifier(interp, u);   // sets an error
			goto error;
		}

		if (u.val[u.len-1] == ':') {   // an option
			opts = true;
			u.len--;    // ignore ':'
			if (!check_identifier(interp, u))
				goto error;

			struct Object *tmp = stringobject_newfromustr_copy(interp, u);
			if (!tmp)
				goto error;
			bool ok = arrayobject_push(interp, optnames, tmp);
			OBJECT_DECREF(interp, tmp);
			if (!ok)
				goto error;
		} else {
			if (opts) {
				// TODO: should any order be allowed when defining the function? it's allowed when calling
				errorobject_throwfmt(interp, "ArgError", "argument \"%U\" should be before options", u);
				goto error;
			}
			if (!check_identifier(interp, u))
				goto error;

			// no need to create another string object, u wasn't modified
			if (!arrayobject_push(interp, argnames, ARRAYOBJECT_GET(splitted, i)))
				goto error;
		}
	}

	OBJECT_DECREF(interp, splitted);
	return true;

error:
	OBJECT_DECREF(interp, splitted);
	return false;
}

static struct LambdaData *create_ldata(struct Interpreter *interp, struct Object *argnames, struct Object *optnames, struct Object *block)
{
	struct LambdaData *ldata = malloc(sizeof(struct LambdaData));
	if (!ldata) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	if (!(ldata->argtypes = arrayobject_newwithcapacity(interp, ARRAYOBJECT_LEN(argnames)))) {
		free(ldata);
		return NULL;
	}
	for (size_t i=0; i < ARRAYOBJECT_LEN(argnames); i++) {
		if (!arrayobject_push(interp, ldata->argtypes, interp->builtins.Object)) {
			OBJECT_DECREF(interp, ldata->argtypes);
			free(ldata);
			return NULL;
		}
	}

	if (!(ldata->opttypes = mappingobject_newempty(interp))) {
		OBJECT_DECREF(interp, ldata->argtypes);
		free(ldata);
		return NULL;
	}
	for (size_t i=0; i < ARRAYOBJECT_LEN(optnames); i++) {
		if (!mappingobject_set(interp, ldata->opttypes, ARRAYOBJECT_GET(optnames, i), interp->builtins.Object)) {
			OBJECT_DECREF(interp, ldata->argtypes);
			OBJECT_DECREF(interp, ldata->opttypes);
			free(ldata);
			return NULL;
		}
	}

	ldata->argnames = argnames;
	OBJECT_INCREF(interp, argnames);
	ldata->optnames = optnames;
	OBJECT_INCREF(interp, optnames);
	ldata->block = block;
	OBJECT_INCREF(interp, block);
	return ldata;
}

struct Object *lambdabuiltin(struct Interpreter *interp, struct ObjectData dummydata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *argstr = ARRAYOBJECT_GET(args, 0);
	struct Object *block = ARRAYOBJECT_GET(args, 1);

	struct Object *argnames = arrayobject_newempty(interp);
	if (!argnames)
		return NULL;
	struct Object *optnames = arrayobject_newempty(interp);
	if (!optnames) {
		OBJECT_DECREF(interp, argnames);
		return NULL;
	}
	if (!parse_arg_and_opt_names(interp, argstr, argnames, optnames)) {
		OBJECT_DECREF(interp, argnames);
		OBJECT_DECREF(interp, optnames);
		return NULL;
	}

	struct LambdaData *ldata = create_ldata(interp, argnames, optnames, block);
	OBJECT_DECREF(interp, argnames);
	OBJECT_DECREF(interp, optnames);
	if (!ldata)
		return NULL;

	struct Object *func = functionobject_new(interp, (struct ObjectData){.data=ldata, .foreachref=ldata_foreachref, .destructor=ldata_destructor}, runner, "a lambda function");
	if (!func) {
		// TODO: writing these here sucks, create a function that can free any ObjectData correctly
		OBJECT_DECREF(interp, ldata->argnames);
		OBJECT_DECREF(interp, ldata->optnames);
		OBJECT_DECREF(interp, ldata->argtypes);
		OBJECT_DECREF(interp, ldata->opttypes);
		OBJECT_DECREF(interp, ldata->block);
		free(ldata);
		return NULL;
	}
	return func;
}
