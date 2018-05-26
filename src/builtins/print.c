#include "print.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../check.h"
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"
#include "../objects/array.h"
#include "../objects/errors.h"
#include "../objects/function.h"
#include "../objects/string.h"
#include "../unicode.h"

struct Object *builtin_print(struct Interpreter *interp, struct Object *argarr)
{
	if (check_args(interp, argarr, interp->builtins.stringclass, NULL) == STATUS_ERROR)
		return NULL;

	char *utf8;
	size_t utf8len;
	char errormsg[100];
	int status = utf8_encode(*((struct UnicodeString *) ARRAYOBJECT_GET(argarr, 0)->data), &utf8, &utf8len, errormsg);
	if (status == STATUS_NOMEM) {
		errorobject_setnomem(interp);
		return NULL;
	}
	assert(status == STATUS_OK);  // TODO: how about invalid unicode strings? make sure they don't exist when creating strings?

	// TODO: avoid writing 1 byte at a time... seems to be hard with c \0 strings
	for (size_t i=0; i < utf8len; i++)
		putchar(utf8[i]);
	free(utf8);
	putchar('\n');

	// this must return a new reference on success
	return stringobject_newfromcharptr(interp, "asd");
}
