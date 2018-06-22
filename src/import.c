#include "import.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/errors.h"
#include "objects/scope.h"
#include "parse.h"
#include "run.h"
#include "tokenizer.h"
#include "unicode.h"

#if defined(_WIN32) || defined(_WIN64)
#define SLASH "\\"
#else
#define SLASH "/"
#endif


// TODO: make this less shitty
char *import_findstdlib(char *argv0)
{
	char *res = malloc(sizeof("stdlib"));
	if (!res) {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		return NULL;
	}

	memcpy(res, "stdlib", sizeof("stdlib"));
	return res;
}


static bool read_and_run_file(struct Interpreter *interp, char *path, struct Object *scope, bool runningbuiltinsfile)
{
	// ValueError can't be used when running builtins.ö because it's defined in builtins.ö
	// TODO: create an IOError class or something?
	char *ioerrorclass = runningbuiltinsfile ? "Error" : "ValueError";

	// read the file
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		errorobject_throwfmt(interp, ioerrorclass, "cannot open '%s': %s", path, strerror(errno));
		return false;
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
			return false;
		}
		huge = ptr;
		memcpy(huge+hugelen, buf, nread);
		hugelen += nread;
	}

	if (ferror(f)) {    // ferror doesn't set errno
		// TODO: use a more appropriate error class if/when it will be available
		errorobject_throwfmt(interp, ioerrorclass, "an error occurred while reading '%s': %s", path, strerror(errno));
		free(huge);
		return false;
	}

	if (fclose(f) != 0) {
		errorobject_throwfmt(interp, ioerrorclass, "an error occurred while closing a file opened from '%s': %s", path, strerror(errno));
		free(huge);
		return false;
	}

	// convert to UnicodeString
	struct UnicodeString code;
	bool ok = utf8_decode(interp, huge, hugelen, &code);
	free(huge);
	if (!ok)
		return false;

	// tokenize
	// TODO: handle errors in tokenizer.c :(
	struct Token *tok1st = token_ize(interp, code);
	free(code.val);

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

		ok = arrayobject_push(interp, statements, stmtnode);
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
		ok = run_statement(interp, scope, ARRAYOBJECT_GET(statements, i));
		stack_pop(interp);
		if (!ok) {
			OBJECT_DECREF(interp, statements);
			return false;
		}
	}

	OBJECT_DECREF(interp, statements);
	return true;
}


bool import_runbuiltinsfile(struct Interpreter *interp)
{
	char path[strlen(interp->stdlibpath) + sizeof(SLASH)-1 + sizeof("builtins.ö") /* includes \0 */];
	strcpy(path, interp->stdlibpath);
	strcat(path, SLASH "builtins.ö");
	return read_and_run_file(interp, path, interp->builtinscope, true);
}

bool import_runmainfile(struct Interpreter *interp, char *path)
{
	struct Object *subscope = scopeobject_newsub(interp, interp->builtinscope);
	if (!subscope)
		return false;

	bool ok = read_and_run_file(interp, path, subscope, false);
	OBJECT_DECREF(interp, subscope);
	return ok;
}
