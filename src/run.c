#include "run.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/errors.h"
#include "objects/scope.h"
#include "objectsystem.h"
#include "parse.h"
#include "path.h"
#include "runast.h"
#include "tokenizer.h"
#include "utf8.h"


static int read_and_run_file(struct Interpreter *interp, char *path, struct Object *scope, bool runningbuiltinsfile, bool handleENOENT)
{
	assert(path_isabsolute(path));

	// ValueError can't be used when running builtins.ö because it's defined in builtins.ö
	// TODO: create an IOError class or something?
	char *ioerrorclass = runningbuiltinsfile ? "Error" : "ValueError";

	// read the file
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		// both windows and posix have ENOENT even though C99 doesn't have it
		if (errno == ENOENT && handleENOENT) {
			errno = 0;
			return -1;
		}
		errorobject_throwfmt(interp, ioerrorclass, "cannot open '%s': %s", path, strerror(errno));
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
			// free(NULL) does nothing
			free(huge);
			errorobject_thrownomem(interp);
			return 0;
		}
		huge = ptr;
		memcpy(huge+hugelen, buf, nread);
		hugelen += nread;
	}

	if (ferror(f)) {    // ferror doesn't set errno
		// TODO: use a more appropriate error class if/when it will be available
		errorobject_throwfmt(interp, ioerrorclass, "an error occurred while reading '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	if (fclose(f) != 0) {
		errorobject_throwfmt(interp, ioerrorclass, "an error occurred while closing a file opened from '%s': %s", path, strerror(errno));
		free(huge);
		return 0;
	}

	// convert to UnicodeString
	struct UnicodeString code;
	bool ok = utf8_decode(interp, huge, hugelen, &code);
	free(huge);
	if (!ok)
		return 0;

	// tokenize
	// TODO: handle errors in tokenizer.c :(
	struct Token *tok1st = token_ize(interp, code);
	free(code.val);

	// parse
	struct Object *statements = arrayobject_newempty(interp);
	if (!statements) {
		token_freeall(tok1st);
		return 0;
	}

	struct Token *curtok = tok1st;
	while (curtok) {
		struct Object *stmtnode = parse_statement(interp, path, &curtok);
		if (!stmtnode) {
			token_freeall(tok1st);
			OBJECT_DECREF(interp, statements);
			return 0;
		}

		ok = arrayobject_push(interp, statements, stmtnode);
		OBJECT_DECREF(interp, stmtnode);
		if (!ok) {
			token_freeall(tok1st);
			OBJECT_DECREF(interp, statements);
			return 0;
		}
	}
	token_freeall(tok1st);

	// run
	for (size_t i=0; i < ARRAYOBJECT_LEN(statements); i++) {
		struct AstNodeObjectData *astdata = ARRAYOBJECT_GET(statements, i)->data;
		if (!stack_push(interp, path, astdata->lineno, scope)) {
			OBJECT_DECREF(interp, statements);
			return 0;
		}
		ok = runast_statement(interp, scope, ARRAYOBJECT_GET(statements, i));
		stack_pop(interp);
		if (!ok) {
			OBJECT_DECREF(interp, statements);
			return 0;
		}
	}

	OBJECT_DECREF(interp, statements);
	return 1;
}

bool run_builtinsfile(struct Interpreter *interp)
{
	// interp->stdpath should be absolute
	char *path = path_concat(interp->stdpath, "builtins.ö");
	if (!path) {
		errorobject_thrownomem(interp);
		return false;
	}

	int status = read_and_run_file(interp, path, interp->builtinscope, true, false);
	free(path);
	assert(status >= 0);
	return (bool)status;
}


int run_libfile(struct Interpreter *interp, char *abspath, struct Object **res)
{
	assert(path_isabsolute(abspath));

	struct Object *subscope = scopeobject_newsub(interp, interp->builtinscope);
	if (!subscope)
		return 0;

	int status = read_and_run_file(interp, abspath, subscope, false, true);
	if (status != 1 /* not ok */) {
		OBJECT_DECREF(interp, subscope);
		return status;
	}

	*res = SCOPEOBJECT_LOCALVARS(subscope);
	OBJECT_INCREF(interp, *res);
	OBJECT_DECREF(interp, subscope);
	return 1;
}


bool run_mainfile(struct Interpreter *interp, char *path)
{
	char *abspath = path_toabsolute(path);
	if (!abspath) {
		if (errno == ENOMEM)
			errorobject_thrownomem(interp);
		else    // ValueError sucks for this .....
			errorobject_throwfmt(interp, "ValueError", "cannot get absolute path of '%s': %s", path, strerror(errno));
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
