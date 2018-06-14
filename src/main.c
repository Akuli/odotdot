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
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/scope.h"
#include "parse.h"

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
			errorobject_setnomem(interp);
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
		errorobject_setwithfmt(interp, "ValueError", "an error occurred while reading '%s'", filename);
		return false;
	}

	*dest = s;
	*destlen = totalsize;
	return true;
}


static bool print_ustr(struct Interpreter *interp, struct UnicodeString u, FILE *f)
{
	char *utf8;
	size_t utf8len;
	if (!utf8_encode(interp, u, &utf8, &utf8len))
		return false;

	for (size_t i=0; i < utf8len; i++)
		fputc(utf8[i], f);
	free(utf8);
	return true;
}

// TODO: print a stack trace and use stderr instead of stdout
static void print_and_reset_err(struct Interpreter *interp)
{
	if (!(interp->err)) {
		// no error to print, nothing to reset
		fprintf(stderr, "%s: interp->err wasn't set correctly\n", interp->argv0);
		return;
	}

	if (interp->err == interp->builtins.nomemerr) {
		// no memory must be allocated in this case...
		fprintf(stderr, "MemError: not enough memory\n");
	} else {
		// utf8_encode may set another error
		struct Object *errsave = interp->err;
		interp->err = NULL;

		if (!print_ustr(interp, ((struct ClassObjectData*) errsave->klass->data)->name, stderr)) {
			fprintf(stderr, "failed to display the error\n");
			OBJECT_DECREF(interp, errsave);
			goto end;    // interp->err was set to the utf8_encode error and THAT is cleared
		}
		fputc(':', stderr);
		fputc(' ', stderr);
		if (!print_ustr(interp, *((struct UnicodeString*) ((struct Object*) errsave->data)->data), stderr)) {
			fprintf(stderr, "\nfailed to display the error\n");
			OBJECT_DECREF(interp, errsave);
			goto end;
		}
		OBJECT_DECREF(interp, errsave);
		fputc('\n', stderr);
		return;
	}

end:
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
		struct Object *stmtnode = parse_statement(interp, &curtok);
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
		if (!run_statement(interp, scope, ARRAYOBJECT_GET(statements, i))) {
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
