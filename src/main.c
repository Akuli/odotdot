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
#include "gc.h"
#include "interpreter.h"
#include "run.h"
#include "tokenizer.h"
#include "unicode.h"

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


// function pointers can't be casted conveniently because c standards
static void astnode_free_voidstar(void *node)
{
	astnode_free(node);    // casts the node to struct AstNode*
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

	struct Token *curtok = tok1st;
	while (curtok) {
		struct AstNode *st = parse_statement(&curtok);
		if (!st) {
			// TODO: better error reporting... also fix ast.c
			fprintf(stderr, "%s: something went wrong with parsing the content of '%s'\n", ctx->interp->argv0, path);
			token_freeall(tok1st);
			return 1;
		}
		dynamicarray_push(statements, st);
	}
	token_freeall(tok1st);

	// run!
	struct Object *err = NULL;
	for (size_t i=0; i < statements->len; i++) {
		run_statement(ctx, &err, statements->values[i]);
		if (err) {
			fprintf(stderr, "an error occurred, printing errors is not implemented yet :(\n");
			dynamicarray_freeall(statements, astnode_free_voidstar);
			return 1;
		}
	}

	dynamicarray_freeall(statements, astnode_free_voidstar);
	return 0;
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

	struct Object *setuperr;
	int status = builtins_setup(interp, &setuperr);
	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: not enough memory\n", argv[0]);
		interpreter_free(interp);
		return 1;
	}
	if (status == STATUS_ERROR) {
		fprintf(stderr, "an error occurred, printing errors is not implemented yet :(\n");
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
