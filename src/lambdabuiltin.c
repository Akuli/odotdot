#include "lambdabuiltin.h"
#include <assert.h>
#include <stdbool.h>
#include "attribute.h"
#include "check.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "unicode.h"


// lambda is like func, but it returns the function instead of setting it to a variable
// this is partialled to argument and option name arrays and a block to create lambdas
static struct Object *runner(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	assert(ARRAYOBJECT_LEN(args) >= 3);
	struct Object *argnames = ARRAYOBJECT_GET(args, 0);
	struct Object *optnames = ARRAYOBJECT_GET(args, 1);
	struct Object *block = ARRAYOBJECT_GET(args, 2);
	// argnames, optnames and block must have the correct type because this can be only ran by lambdabuiltin()

	struct Object *theargs = arrayobject_slice(interp, args, 3, ARRAYOBJECT_LEN(args));
	if (!theargs)
		return NULL;

	// create mappings for check_args_with_array and check_opts_with_mapping
	// TODO: partial these in lambdabuiltin(), allocating more memory every time the function runs is bad
	struct Object *argtypes = arrayobject_newempty(interp);
	if (!argtypes) {
		OBJECT_DECREF(interp, theargs);
		return NULL;
	}
	for (size_t i=0; i < ARRAYOBJECT_LEN(argnames); i++) {
		// everything is an Object, so this disables type checking (lol)
		if (!arrayobject_push(interp, argtypes, interp->builtins.Object)) {
			OBJECT_DECREF(interp, argtypes);
			OBJECT_DECREF(interp, theargs);
			return NULL;
		}
	}

	struct Object *opttypes = mappingobject_newempty(interp);
	if (!opttypes) {
		OBJECT_DECREF(interp, argtypes);
		OBJECT_DECREF(interp, theargs);
		return NULL;
	}
	for (size_t i=0; i < ARRAYOBJECT_LEN(optnames); i++) {
		// everything is an Object, so this disables type checking (lol)
		if (!mappingobject_set(interp, opttypes, ARRAYOBJECT_GET(optnames, i), interp->builtins.Object)) {
			OBJECT_DECREF(interp, opttypes);
			OBJECT_DECREF(interp, argtypes);
			OBJECT_DECREF(interp, theargs);
			return NULL;
		}
	}

	bool ok = check_args_with_array(interp, theargs, argtypes) && check_opts_with_mapping(interp, opts, opttypes);
	OBJECT_DECREF(interp, opttypes);
	OBJECT_DECREF(interp, argtypes);
	if (!ok) {
		OBJECT_DECREF(interp, theargs);
		return NULL;
	}

	struct Object *parentscope = attribute_get(interp, block, "definition_scope");
	if (!parentscope) {
		OBJECT_DECREF(interp, theargs);
		return NULL;
	}

	struct Object *scope = scopeobject_newsub(interp, parentscope);
	OBJECT_DECREF(interp, parentscope);
	if (!scope) {
		OBJECT_DECREF(interp, theargs);
		return NULL;
	}

	// add values of arguments...
	for (size_t i=0; i < ARRAYOBJECT_LEN(argnames); i++) {
		if (!mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), ARRAYOBJECT_GET(argnames, i), ARRAYOBJECT_GET(theargs, i))) {
			OBJECT_DECREF(interp, theargs);
			goto error;
		}
	}
	OBJECT_DECREF(interp, theargs);

	// ...and options
	for (size_t i=0; i < ARRAYOBJECT_LEN(optnames); i++) {
		struct Object *optname = ARRAYOBJECT_GET(optnames, i);
		struct Object *val;

		int status = mappingobject_get(interp, opts, optname, &val);
		if (status == -1)
			goto error;
		if (status == 0)    // value not specified, but options are optional ofc :D
			val = nullobject_get(interp);
		assert(val);

		bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), optname, val);
		OBJECT_DECREF(interp, val);
		if (!ok)
			goto error;
	}

	struct Object *retval = blockobject_runwithreturn(interp, block, scope);
	OBJECT_DECREF(interp, scope);
	return retval;

error:
	OBJECT_DECREF(interp, scope);
	return NULL;
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
		struct UnicodeString u = *((struct UnicodeString *) ARRAYOBJECT_GET(splitted, i)->data);

		if (u.len == 0) {
			check_identifier(interp, u);   // sets an error
			goto error;
		}

		if (u.val[u.len-1] == ':') {   // an option
			opts = true;
			u.len--;    // ignore ':'
			if (!check_identifier(interp, u))
				goto error;

			struct Object *tmp = stringobject_newfromustr(interp, u);
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

struct Object *lambdabuiltin(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *block = ARRAYOBJECT_GET(args, 1);

	struct Object *argnames = arrayobject_newempty(interp);
	if (!argnames)
		return NULL;
	struct Object *optnames = arrayobject_newempty(interp);
	if (!optnames) {
		OBJECT_DECREF(interp, argnames);
		return NULL;
	}
	if (!parse_arg_and_opt_names(interp, ARRAYOBJECT_GET(args, 0), argnames, optnames)) {
		OBJECT_DECREF(interp, argnames);
		OBJECT_DECREF(interp, optnames);
		return NULL;
	}

	// it's possible to micro-optimize this by not creating a new runner every time
	// but i don't think it would make a huge difference
	// it doesn't actually affect calling the functions anyway, just defining them
	//
	// the name will be copied when partialling
	struct Object *runnerobj = functionobject_new(interp, runner, "a lambda function");
	if (!runnerobj) {
		OBJECT_DECREF(interp, argnames);
		OBJECT_DECREF(interp, optnames);
		return NULL;
	}

	// TODO: we need a way to partial multiple things easily
	struct Object *partial1 = functionobject_newpartial(interp, runnerobj, argnames);
	OBJECT_DECREF(interp, runnerobj);
	OBJECT_DECREF(interp, argnames);
	if (!partial1) {
		OBJECT_DECREF(interp, optnames);
		return NULL;
	}

	struct Object *partial2 = functionobject_newpartial(interp, partial1, optnames);
	OBJECT_DECREF(interp, partial1);
	OBJECT_DECREF(interp, optnames);
	if (!partial2)
		return NULL;

	struct Object *partial3 = functionobject_newpartial(interp, partial2, block);
	OBJECT_DECREF(interp, partial2);
	return partial3;     // may be NULL
}
