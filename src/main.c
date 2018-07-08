#include <assert.h>
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
#include "../config.h"

// most of the readline code is taken from 'Programming with GNU Readline' in 'info readline'
#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

// returns 1 on success, 0 on EOF and -1 on error (already printed to stderr)
// on success, *line must be free()'d
static int get_line(struct Interpreter *interp, char *prompt, char **line)
{
#ifdef HAVE_READLINE
	char *res = readline(prompt);
	if (!res) {
		fputc('\n', stderr);
		return 0;
	}
	if (res && *res)
		add_history(res);
	*line = res;
	return 1;

#else
	fputs(prompt, stderr);
	fflush(stderr);

	// TODO: handle errors? the man page doesn't say anything, it's just eof and success
#define SIZE 1000   // should be big enough
	char *buf = malloc(SIZE);
	if (!buf) {
		fprintf(stderr, "%s: not enough memory\n", interp->argv0);
		return -1;
	}
	bool eof = !fgets(buf, SIZE, stdin);
#undef SIZE

	if (eof) {
		free(buf);
		fputc('\n', stderr);   // makes it much better!
		return 0;
	}
	*line = buf;
	return 1;

#endif     // HAVE_READLINE
}


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
#ifdef HAVE_READLINE
	rl_bind_key('\t', rl_insert);
#endif

	struct Object *scope = scopeobject_newsub(interp, interp->builtinscope);
	if (!scope)
		return false;

	while (true) {
		char *line;
		int status = get_line(interp, "ö> ", &line);
		if (status == -1 || status == 0) {
			OBJECT_DECREF(interp, scope);
			return (status == 0);   // true on success, false on failure
		}

		struct UnicodeString code;
		bool ok = utf8_decode(interp, line, strlen(line), &code);
		free(line);
		if (!ok) {
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

		ok = run_string(interp, "<repl>", code, scope);
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
	if (argc == 2) {
		ok = run_mainfile(interp, argv[1]);
		if (!ok)
			print_and_reset_err(interp);
	} else
		ok = repl(interp);

	if (!ok)
		returnval = 1;
	// "fall through" to end

end:
	builtins_teardown(interp);
	gc_run(interp);
	interpreter_free(interp);
	return returnval;
}
