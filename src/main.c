#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "builtins.h"
#include "gc.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/errors.h"
#include "objects/scope.h"
#include "run.h"
#include "unicode.h"
#include "utf8.h"


static void print_and_reset_err(struct Interpreter *interp)
{
	assert(interp->err);
	struct Object *errsave = interp->err;
	interp->err = NULL;
	errorobject_print(interp, errsave);
	OBJECT_DECREF(interp, errsave);
}

// more like REL (read-eval-loop) than REPL because printing is missing
static bool repl(struct Interpreter *interp)
{
	struct Object *scope = scopeobject_newsub(interp, interp->builtinscope);
	if (!scope)
		return false;

	char buf[1000];   // TODO: get rid of fixed buffer size? this is quite big though
	while (true) {
		fprintf(stderr, "ö> ");
		fflush(stderr);

		if (!fgets(buf, sizeof(buf), stdin)) {
			if (feof(stdin)) {
				printf("\n");
				OBJECT_DECREF(interp, scope);
				return true;
			}

			// TODO: does fgets actually set errno?
			errorobject_throwfmt(interp, "IoError", "cannot read line from stdin: %s\n", strerror(errno));
			OBJECT_DECREF(interp, scope);
			return false;
		}

		struct UnicodeString code;
		if (!utf8_decode(interp, buf, strlen(buf), &code)) {
			OBJECT_DECREF(interp, scope);
			return false;
		}

		// if the line is nothing but whitespace, do nothing
		bool wsonly = true;
		for (size_t i=0; i<code.len; i++) {
			if (!unicode_isspace(code.val[i])) {
				wsonly = false;
				break;
			}
		}
		if (wsonly)
			continue;

		bool ok = run_string(interp, "<repl>", code, scope);
		free(code.val);
		if (!ok)
			print_and_reset_err(interp);
	}
}

int main(int argc, char **argv)
{
	if (argc > 2) {
		assert(argc >= 1);   // not sure if standards allow 0 args
		fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
		return 2;
	}
	assert(argc == 1 || argc == 2);

	struct Interpreter *interp = interpreter_new(argv[0]);
	if (!interp)
		return 1;

	int returnval = 0;
	if (!builtins_setup(interp)) {
		returnval = 1;
		goto end;
	}

	if (!run_builtinsfile(interp)) {
		print_and_reset_err(interp);
		returnval = 1;
		goto end;
	}

	bool ok;
	if (argc == 2)
		ok = run_mainfile(interp, argv[1]);
	else
		ok = repl(interp);
	if (!ok) {
		print_and_reset_err(interp);
		returnval = 1;
	}

	// "fall through" to end

end:
	builtins_teardown(interp);
	gc_run(interp);
	interpreter_free(interp);
	return returnval;
}
