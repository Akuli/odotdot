#include <assert.h>
#include <errno.h>
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
#include "objects/scope.h"

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

	// no memory must be allocated in this case...
	if (interp->err == interp->builtins.nomemerr) {
		fprintf(stderr, "error: not enough memory\n");
	} else {
		char *utf8;
		size_t utf8len;
		char errormsg[100];
		// TODO: handle no mem
		assert(utf8_encode(*((struct UnicodeString *) ((struct Object *) interp->err->data)->data), &utf8, &utf8len, errormsg) == STATUS_OK);
		fprintf(stderr, "error: ");
		for (size_t i=0; i < utf8len; i++)
			fputc(utf8[i], stderr);
		free(utf8);
		fputc('\n', stderr);
	}

	OBJECT_DECREF(interp, interp->err);
	interp->err = NULL;
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

	int returnval = 0;
	int status = builtins_setup(interp);
	assert(status == STATUS_NOMEM || status == STATUS_ERROR || status == STATUS_OK);
	if (status == STATUS_NOMEM)
		fprintf(stderr, "%s: not enough memory\n", argv[0]);

	if (status != STATUS_OK) {
		returnval = 1;
		goto end;
	}
	if (status == STATUS_ERROR) {
		// an error message has already been printed
		builtins_teardown(interp);
		interpreter_free(interp);
		return 1;
	}
	assert(status == STATUS_OK);

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
