#include <assert.h>
#include "builtins.h"
#include "gc.h"
#include "import.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/errors.h"


static void print_and_reset_err(struct Interpreter *interp)
{
	assert(interp->err);
	struct Object *errsave = interp->err;
	interp->err = NULL;
	errorobject_print(interp, errsave);
	OBJECT_DECREF(interp, errsave);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		assert(argc >= 1);   // not sure if standards allow 0 args
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		return 2;
	}

	struct Interpreter *interp = interpreter_new(argv[0]);
	if (!interp)
		return 1;

	int returnval = 0;
	if (!builtins_setup(interp)) {
		returnval = 1;
		goto end;
	}

	if (!import_runbuiltinsfile(interp)) {
		print_and_reset_err(interp);
		returnval = 1;
		goto end;
	}
	if (!import_runmainfile(interp, argv[1])) {
		print_and_reset_err(interp);
		returnval = 1;
		goto end;
	}

	// "fall through" to end

end:
	builtins_teardown(interp);
	gc_run(interp);
	interpreter_free(interp);
	return returnval;
}
