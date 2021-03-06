#include "run.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
#include "interpreter.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objectsystem.h"
#include "parse.h"
#include "path.h"
#include "runast.h"
#include "stack.h"
#include "tokenizer.h"
#include "unicode.h"
#include "utf8.h"

// see Makefile
static unicode_char builtinscode[] = {
#include "builtinscode.h"      // IWYU pragma: keep
};


static bool run(struct Interpreter *interp, char *path, struct UnicodeString ucode, struct Object *scope, bool runningbuiltinsfile)
{
	// tokenize
	// TODO: handle errors in tokenizer.c :(
	struct Token *tok1st = token_ize(interp, ucode);

	// parse
	struct Object *statements = arrayobject_newempty(interp);
	if (!statements) {
		token_freeall(tok1st);
		return false;
	}

	struct Token *curtok = tok1st;
	while (curtok) {
		struct Object *stmtnode = parse_statement(interp, path, &curtok);
		if (!stmtnode) {
			token_freeall(tok1st);
			OBJECT_DECREF(interp, statements);
			return false;
		}

		bool ok = arrayobject_push(interp, statements, stmtnode);
		OBJECT_DECREF(interp, stmtnode);
		if (!ok) {
			token_freeall(tok1st);
			OBJECT_DECREF(interp, statements);
			return false;
		}
	}
	token_freeall(tok1st);

	// run
	for (size_t i=0; i < ARRAYOBJECT_LEN(statements); i++) {
		struct AstNodeObjectData *astdata = ARRAYOBJECT_GET(statements, i)->objdata.data;
		if (!stack_push(interp, path, astdata->lineno, scope)) {
			OBJECT_DECREF(interp, statements);
			return false;
		}
		bool ok = runast_statement(interp, scope, ARRAYOBJECT_GET(statements, i));
		stack_pop(interp);
		if (!ok) {
			OBJECT_DECREF(interp, statements);
			return false;
		}
	}

	OBJECT_DECREF(interp, statements);
	return true;
}

bool run_string(struct Interpreter *interp, char *filepath, struct UnicodeString code, struct Object *scope)
{
	return run(interp, filepath, code, scope, false);
}

bool run_builtinsfile(struct Interpreter *interp)
{
	struct UnicodeString code = { .len = sizeof(builtinscode)/sizeof(builtinscode[0]), .val = builtinscode };
	return run(interp, "<builtins>", code, interp->builtinscope, true);
}


static int read_and_run_file(struct Interpreter *interp, char *path, struct Object *scope, bool runningbuiltinsfile, bool handleENOENT)
{
	assert(path_isabsolute(path));

	// read the file
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		// both windows and posix have ENOENT even though C99 doesn't have it
		if (errno == ENOENT && handleENOENT) {
			errno = 0;
			return -1;
		}
		// IoError must be there because it's from builtins.ö, and that runs funnily without this
		errorobject_throwfmt(interp, "IoError", "cannot open '%s': %s", path, strerror(errno));
		return 0;
	}

	char *huge = NULL;
	char buf[4096];
	size_t hugelen=0;
	size_t nread;
	while ((nread = fread(buf, 1, sizeof(buf), f))) {
		// <jp> if s is NULL in realloc(s, sz), it acts like malloc
		char *ptr = realloc(huge, hugelen+nread);
		if (!ptr) {
			free(huge);   // free(NULL) does nothing
			errorobject_thrownomem(interp);
			return 0;
		}
		huge = ptr;
		memcpy(huge+hugelen, buf, nread);
		hugelen += nread;
	}

	if (ferror(f)) {    // ferror doesn't set errno
		errorobject_throwfmt(interp, "IoError", "cannot read '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	if (fclose(f) != 0) {
		errorobject_throwfmt(interp, "IoError", "cannot close a file opened from '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	struct UnicodeString uhuge;
	bool ok = utf8_decode(interp, huge, hugelen, &uhuge);
	free(huge);
	if (!ok)
		return false;

	int res = run(interp, path, uhuge, scope, runningbuiltinsfile);
	free(uhuge.val);
	return res;
}

static void libdata_foreachref(void *data, object_foreachrefcb cb, void *cbdata)
{
	cb((struct Object*)data, cbdata);
}

static bool export_cfunc(struct Interpreter *interp, struct ObjectData libdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *block = ARRAYOBJECT_GET(args, 0);
	struct Object *scope = BLOCKOBJECT_DEFSCOPE(block);
	struct Object *lib = libdata.data;

	// keys are variable names that existed when this ran, values are none
	// this is used for checking which variables were added during the export
	struct Object *oldvars = mappingobject_newempty(interp);
	if (!oldvars)
		return false;

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, SCOPEOBJECT_LOCALVARS(scope));
	while (mappingobject_iternext(&iter)) {
		if (!mappingobject_set(interp, oldvars, iter.key, interp->builtins.none))
			goto error;
	}

	if (!blockobject_run(interp, block, scope))
		goto error;

	mappingobject_iterbegin(&iter, SCOPEOBJECT_LOCALVARS(scope));
	while (mappingobject_iternext(&iter)) {
		// check if the var is is in oldvars
		struct Object *tmp;
		int res = mappingobject_get(interp, oldvars, iter.key, &tmp);
		if (res == -1)
			goto error;
		if (res == 1) {
			// it is an old var, not defined in export and not meant to be exported
			OBJECT_DECREF(interp, tmp);
			continue;
		}
		assert(res == 0);

		if (!attribute_setwithstringobj(interp, lib, iter.key, iter.value))
			goto error;
	}

	OBJECT_DECREF(interp, oldvars);
	return true;

error:
	OBJECT_DECREF(interp, oldvars);
	return false;
}

int run_libfile(struct Interpreter *interp, char *abspath, struct Object *lib)
{
	struct Object *scope = scopeobject_newsub(interp, interp->builtinscope);
	if (!scope)
		return 0;

	struct Object *export = functionobject_new(interp, (struct ObjectData){.data=lib, .foreachref=libdata_foreachref, .destructor=NULL}, functionobject_mkcfunc_noret(export_cfunc), "export");
	if (!export) {
		OBJECT_DECREF(interp, scope);
		return 0;
	}
	OBJECT_INCREF(interp, lib);   // for export

	bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), interp->strings.export, export);
	OBJECT_DECREF(interp, export);
	if (!ok) {
		OBJECT_DECREF(interp, scope);
		return 0;
	}

	int status = read_and_run_file(interp, abspath, scope, false, true);
	OBJECT_DECREF(interp, scope);
	return status;
}


bool run_mainfile(struct Interpreter *interp, char *path)
{
	char *abspath = path_toabsolute(path);
	if (!abspath) {
		if (errno == ENOMEM)
			errorobject_thrownomem(interp);
		else
			errorobject_throwfmt(interp, "IoError", "cannot get absolute path of '%s': %s", path, strerror(errno));
		return false;
	}

	struct Object *subscope = scopeobject_newsub(interp, interp->builtinscope);
	if (!subscope) {
		free(abspath);
		return false;
	}

	int status = read_and_run_file(interp, abspath, subscope, false, false);
	free(abspath);
	OBJECT_DECREF(interp, subscope);
	assert(status == !!status);
	return status;
}
