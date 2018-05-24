#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "builtins.h"
#include "common.h"
#include "gc.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "run.h"
#include "tokenizer.h"
#include "unicode.h"
#include "objects/array.h"
#include "objects/function.h"

#define FILE_CHUNK_SIZE 4096


// returns STATUS_OK, STATUS_NOMEM or 1 for a reading error
int read_file_to_huge_string(FILE *f, char **dest, size_t *destlen)
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
			return STATUS_NOMEM;
		}
		s = ptr;
		memcpy(s+totalsize, buf, nread);
		totalsize += nread;
	}

	if (!feof(f)) {
		free(s);
		return 1;
	}

	*dest = s;
	*destlen = totalsize;
	return STATUS_OK;
}


// TODO: print a stack trace and use stderr instead of stdout
static void print_and_reset_err(struct Interpreter *interp)
{
	if (!(interp->err)) {
		// no error to print, nothing to reset
		fprintf(stderr, "%s: interp->err wasn't set correctly\n", interp->argv0);
		return;
	}

	struct Object *err = interp->err;
	interp->err = NULL;

	printf("errör: ");
	struct Object *printres = functionobject_call(interp, interp->builtins.print, (struct Object *) err->data, NULL);
	OBJECT_DECREF(interp, err);
	if (!printres) {
		fprintf(stderr, "%s: printing an error failed\n", interp->argv0);
		OBJECT_DECREF(interp, interp->err);
		return;
	}
	OBJECT_DECREF(interp, printres);
}


// prints errors to stderr
// returns an exit code, e.g. 0 for success
static int run_file(struct Interpreter *interp, struct Object *scope, char *path)
{
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		fprintf(stderr, "%s: cannot open '%s'\n", interp->argv0, path);
		return 1;
	}

	char *hugestr;
	size_t hugestrlen;
	int status = read_file_to_huge_string(f, &hugestr, &hugestrlen);
	fclose(f);

	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while reading '%s'\n", interp->argv0, path);
		return 1;
	}
	if (status == 1) {
		fprintf(stderr, "%s: reading '%s' failed\n", interp->argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// convert to UnicodeString
	struct UnicodeString hugeunicode;
	char errormsg[100];
	status = utf8_decode(hugestr, hugestrlen, &hugeunicode, errormsg);
	free(hugestr);

	if (status == 1) {
		fprintf(stderr, "%s: the content of '%s' is not valid UTF-8: %s\n", interp->argv0, path, errormsg);
		return 1;
	}
	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while UTF8-decoding the content of '%s'\n", interp->argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// tokenize
	struct Token *tok1st = token_ize(hugeunicode);   // TODO: handle errors in tokenizer.c :(
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
		struct Object *stmtnode = ast_parse_statement(interp, &curtok);
		if (!stmtnode) {
			print_and_reset_err(interp);
			token_freeall(tok1st);
			returnval = 1;
			goto end;
		}

		int status = arrayobject_push(interp, statements, stmtnode);
		OBJECT_DECREF(interp, stmtnode);
		if (status == STATUS_ERROR) {
			print_and_reset_err(interp);
			token_freeall(tok1st);
			returnval = 1;
			goto end;
		}
	}
	token_freeall(tok1st);

	// run!
	for (size_t i=0; i < ARRAYOBJECT_LEN(statements); i++) {
		if (run_statement(interp, scope, ARRAYOBJECT_GET(statements, i)) == STATUS_ERROR) {
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

	int status = builtins_setup(interp);
	if (status == STATUS_NOMEM) {
		builtins_teardown(interp);
		fprintf(stderr, "%s: not enough memory\n", argv[0]);
		interpreter_free(interp);
		return 1;
	}
	if (status == STATUS_ERROR) {
		// an error message has already been printed
		builtins_teardown(interp);
		interpreter_free(interp);
		return 1;
	}
	assert(status == STATUS_OK);

	// TODO: run stdlib/fake_builtins.ö and create a new subscope for this file
	int res = run_file(interp, interp->builtinscope, argv[1]);
	builtins_teardown(interp);
	gc_run(interp);    // remove reference cycles
	interpreter_free(interp);
	return res;
}
