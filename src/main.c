#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtins.h"
#include "gc.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "run.h"
#include "tokenizer.h"
#include "unicode.h"
#include "objects/array.h"
#include "objects/astnode.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/scope.h"
#include "parse.h"
#include "stack.h"

#define FILE_CHUNK_SIZE 4096


// returns false on error
bool read_file_to_huge_string(struct Interpreter *interp, char *filename, FILE *f, char **dest, size_t *destlen)
{
	char *s = NULL;
	char buf[FILE_CHUNK_SIZE];
	size_t totalsize=0;
	size_t nread;
	while ((nread = fread(buf, 1, FILE_CHUNK_SIZE, f))) {
		// <jp> if s is NULL in realloc(s, sz), it acts like malloc
		char *ptr = realloc(s, totalsize+nread);
		if (!ptr) {
			// free(NULL) does nothing
			free(s);
			errorobject_thrownomem(interp);
			return false;
		}
		s = ptr;
		memcpy(s+totalsize, buf, nread);
		totalsize += nread;
	}

	if (!feof(f)) {
		free(s);
		// TODO: use a more appropriate error class if/when it will be available
		// TODO: include more stuff in the error message, maybe use errno and strerror stuff
		errorobject_throwfmt(interp, "ValueError", "an error occurred while reading '%s'", filename);
		return false;
	}

	*dest = s;
	*destlen = totalsize;
	return true;
}


// TODO: print a stack trace and use stderr instead of stdout
static void print_and_reset_err(struct Interpreter *interp)
{
	assert(interp->err);
	struct Object *errsave = interp->err;
	interp->err = NULL;
	errorobject_printsimple(interp, errsave);
	OBJECT_DECREF(interp, errsave);
}


// prints errors to stderr
// returns an exit code, e.g. 0 for success
static int run_file(struct Interpreter *interp, struct Object *scope, char *path)
{
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		fprintf(stderr, "%s: cannot open '%s': %s\n", interp->argv0, path, strerror(errno));
		return 1;
	}

	char *hugestr;
	size_t hugestrlen;
	bool ok = read_file_to_huge_string(interp, path, f, &hugestr, &hugestrlen);
	fclose(f);
	if (!ok) {
		print_and_reset_err(interp);
		return 1;
	}

	// convert to UnicodeString
	struct UnicodeString hugeunicode;
	ok = utf8_decode(interp, hugestr, hugestrlen, &hugeunicode);
	free(hugestr);
	if (!ok) {
		print_and_reset_err(interp);
		return 1;
	}

	// tokenize
	struct Token *tok1st = token_ize(interp, hugeunicode);   // TODO: handle errors in tokenizer.c :(
	free(hugeunicode.val);

	// parse
	struct Object *statements = arrayobject_newempty(interp);
	if (!statements) {
		print_and_reset_err(interp);
		token_freeall(tok1st);
		return 1;
	}

	int returnval = 0;

	struct Token *curtok = tok1st;
	while (curtok) {
		struct Object *stmtnode = parse_statement(interp, path, &curtok);
		if (!stmtnode) {
			print_and_reset_err(interp);
			token_freeall(tok1st);
			returnval = 1;
			goto end;
		}

		ok = arrayobject_push(interp, statements, stmtnode);
		OBJECT_DECREF(interp, stmtnode);
		if (!ok) {
			print_and_reset_err(interp);
			token_freeall(tok1st);
			returnval = 1;
			goto end;
		}
	}
	token_freeall(tok1st);

	// run!
	for (size_t i=0; i < ARRAYOBJECT_LEN(statements); i++) {
		struct AstNodeObjectData *astdata = ARRAYOBJECT_GET(statements, i)->data;
		if (!stack_push(interp, path, astdata->lineno, scope)) {
			print_and_reset_err(interp);
			returnval = 1;
			goto end;
		}
		ok = run_statement(interp, scope, ARRAYOBJECT_GET(statements, i));
		stack_pop(interp);
		if (!ok) {
			print_and_reset_err(interp);
			returnval = 1;
			goto end;
		}
	}
	// "fall through" to end

end:
	OBJECT_DECREF(interp, statements);
	return returnval;
}


int main(int argc, char **argv)
{
	if (argc != 2) {
		assert(argc >= 1);   // not sure if standards allow 0 args
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		return 2;
	}

	struct Interpreter *interp = interpreter_new(argv[0]);
	if (!interp) {
		fprintf(stderr, "%s: not enough memory\n", argv[0]);
		return 1;
	}

	int returnval = 0;
	if (!builtins_setup(interp)) {
		returnval = 1;
		goto end;
	}

	// TODO: find stdlib better, e.g. don't rely on current working directory
	// TODO: use a backslash on windows
	returnval = run_file(interp, interp->builtinscope, "stdlib/builtins.ö");
	if (returnval != 0)
		goto end;

	struct Object *subscope = scopeobject_newsub(interp, interp->builtinscope);
	if (!subscope) {
		print_and_reset_err(interp);
		returnval = 1;
		goto end;
	}

	returnval = run_file(interp, subscope, argv[1]);
	OBJECT_DECREF(interp, subscope);
	// "fall through" to end

end:
	builtins_teardown(interp);
	gc_run(interp);
	interpreter_free(interp);
	return returnval;
}
