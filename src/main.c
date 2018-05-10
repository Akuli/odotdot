#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "builtins.h"
#include "common.h"
#include "context.h"
#include "dynamicarray.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "run.h"
#include "tokenizer.h"
#include "unicode.h"
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
static void print_error(struct Context *ctx, struct Object *err)
{
	struct Object *err2 = NULL;

	struct Object *printfunc = interpreter_getbuiltin(ctx->interp, &err2, "print");
	if (!printfunc) {
		fprintf(stderr, "%s: printing an error failed\n", ctx->interp->argv0);
		OBJECT_DECREF(ctx->interp, err2);
		return;
	}

	printf("errör: ");
	struct Object *printres = functionobject_call(ctx, &err2, printfunc, err->data, NULL);
	OBJECT_DECREF(ctx->interp, printfunc);
	if (!printres) {
		fprintf(stderr, "%s: printing an error failed\n", ctx->interp->argv0);
		OBJECT_DECREF(ctx->interp, err2);
		return;
	}
	OBJECT_DECREF(ctx->interp, printres);
}


// prints errors to stderr
// returns an exit code, e.g. 0 for success
static int run_file(struct Context *ctx, char *path)
{
	FILE *f = fopen(path, "rb");   // b because the content is decoded below... i think this is good
	if (!f) {
		fprintf(stderr, "%s: cannot open '%s'\n", ctx->interp->argv0, path);
		return 1;
	}

	char *hugestr;
	size_t hugestrlen;
	int status = read_file_to_huge_string(f, &hugestr, &hugestrlen);
	fclose(f);

	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while reading '%s'\n", ctx->interp->argv0, path);
		return 1;
	}
	if (status == 1) {
		fprintf(stderr, "%s: reading '%s' failed\n", ctx->interp->argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// convert to UnicodeString
	struct UnicodeString hugeunicode;
	char errormsg[100];
	status = utf8_decode(hugestr, hugestrlen, &hugeunicode, errormsg);
	free(hugestr);

	if (status == 1) {
		fprintf(stderr, "%s: the content of '%s' is not valid UTF-8: %s\n", ctx->interp->argv0, path, errormsg);
		return 1;
	}
	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while UTF8-decoding the content of '%s'\n", ctx->interp->argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// tokenize
	struct Token *tok1st = token_ize(hugeunicode);   // TODO: handle errors in tokenizer.c :(
	free(hugeunicode.val);

	// parse
	struct DynamicArray *statements = dynamicarray_new();
	if (!statements) {
		fprintf(stderr, "%s: ran out of memory while parsing the content of '%s'\n", ctx->interp->argv0, path);
		token_freeall(tok1st);
		return 1;
	}

	int returnval = 0;

	struct Object *err = NULL;
	struct Token *curtok = tok1st;
	while (curtok) {
		struct Object *stmtnode = ast_parse_statement(ctx->interp, &err, &curtok);
		if (!stmtnode) {
			// TODO: better error reporting :(
			assert(err);
			fprintf(stderr, "%s: something went wrong with parsing the content of '%s'\n", ctx->interp->argv0, path);
			OBJECT_DECREF(ctx->interp, err);
			token_freeall(tok1st);
			returnval = 1;
			goto end;
		}
		dynamicarray_push(statements, stmtnode);
	}
	token_freeall(tok1st);

	// run!
	for (size_t i=0; i < statements->len; i++) {
		if (run_statement(ctx, &err, statements->values[i]) == STATUS_ERROR) {
			if (err) {
				print_error(ctx, err);
				OBJECT_DECREF(ctx->interp, err);
			} else {
				fprintf(stderr, "%s: errptr wasn't set correctly\n", ctx->interp->argv0);
			}
			returnval = 1;
			goto end;
		}
	}
	// "fall through" to end

end:
	for (size_t i=0; i < statements->len; i++)
		OBJECT_DECREF(ctx->interp, statements->values[i]);
	dynamicarray_free(statements);
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

	// TODO: run stdlib/fake_builtins.ö and create a new subcontext for this file
	int res = run_file(interp->builtinctx, argv[1]);
	//gc_run(interp);
	builtins_teardown(interp);
	interpreter_free(interp);
	return res;
}
