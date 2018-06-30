#include "run.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
#include "interpreter.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/block.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/scope.h"
#include "objects/string.h"
#include "objectsystem.h"
#include "parse.h"
#include "path.h"
#include "runast.h"
#include "tokenizer.h"
#include "utf8.h"

// see Makefile
// this uses unsigned char instead of char because char may be signed, and utf8 Ö doesn't fit in signed char
const unsigned char builtinscode[] = {
#include "builtinscode.h"
};


static bool run_code(struct Interpreter *interp, char *path, char *code, size_t codelen, struct Object *scope, bool runningbuiltinsfile)
{
	// convert to UnicodeString
	struct UnicodeString ucode;
	if (!utf8_decode(interp, code, codelen, &ucode))
		return false;

	// tokenize
	// TODO: handle errors in tokenizer.c :(
	struct Token *tok1st = token_ize(interp, ucode);
	free(ucode.val);

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
		struct AstNodeObjectData *astdata = ARRAYOBJECT_GET(statements, i)->data;
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

bool run_builtinsfile(struct Interpreter *interp)
{
	// TODO: avoid casting unsigned char* to char* ?
	// i'm not sure if that's guaranteed to work correctly, but rest of my code casts unsigned char to char
	// and that is guaranteed to work: if char is signed, some of the values will be negative
	return run_code(interp, "<builtins>", (char *) builtinscode, sizeof(builtinscode), interp->builtinscope, true);
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
		// TODO: use a more appropriate error class if/when it will be available
		errorobject_throwfmt(interp, "IoError", "cannot read '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	if (fclose(f) != 0) {
		errorobject_throwfmt(interp, "IoError", "cannot close a file opened from '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	int res = run_code(interp, path, huge, hugelen, scope, runningbuiltinsfile);
	free(huge);
	return res;
}

// this is partialled to a Library
static struct Object *export_cfunc(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Library, interp->builtins.Block, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *lib = ARRAYOBJECT_GET(args, 0);
	struct Object *block = ARRAYOBJECT_GET(args, 1);

	struct Object *filescope = attribute_get(interp, block, "definition_scope");
	if (!filescope)
		return NULL;

	struct Object *subscope = scopeobject_newsub(interp, filescope);
	if (!subscope) {
		OBJECT_DECREF(interp, filescope);
		return NULL;
	}

	if (!blockobject_run(interp, block, subscope))
		goto error;

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, SCOPEOBJECT_LOCALVARS(subscope));
	while (mappingobject_iternext(&iter)) {
		if (!attribute_setwithstringobj(interp, lib, iter.key, iter.value))
			goto error;
		if (!mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(filescope), iter.key, iter.value))
			goto error;
	}

	OBJECT_DECREF(interp, subscope);
	OBJECT_DECREF(interp, filescope);
	return nullobject_get(interp);

error:
	OBJECT_DECREF(interp, subscope);
	OBJECT_DECREF(interp, filescope);
	return NULL;
}

int run_libfile(struct Interpreter *interp, char *abspath, struct Object *lib)
{
	struct Object *scope = scopeobject_newsub(interp, interp->builtinscope);
	if (!scope)
		return 0;

	struct Object *exportstr = stringobject_newfromcharptr(interp, "export");
	if (!exportstr) {
		OBJECT_DECREF(interp, scope);
		return 0;
	}

	struct Object *rawexport = functionobject_new(interp, export_cfunc, "export");
	if (!rawexport) {
		OBJECT_DECREF(interp, exportstr);
		OBJECT_DECREF(interp, scope);
		return 0;
	}

	struct Object *export = functionobject_newpartial(interp, rawexport, lib);
	OBJECT_DECREF(interp, rawexport);
	if (!export) {
		OBJECT_DECREF(interp, exportstr);
		OBJECT_DECREF(interp, scope);
	}

	bool ok = mappingobject_set(interp, SCOPEOBJECT_LOCALVARS(scope), exportstr, export);
	OBJECT_DECREF(interp, exportstr);
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
