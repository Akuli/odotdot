#include "interpreter.h"
#include <assert.h>
#include <stdlib.h>
#include "hashtable.h"
#include "objects/string.h"

// mostly copy/pasted from objectsystem.c
// TODO: check types?
static int compare_string_objects(void *a, void *b, void *userdata)
{
        assert(!userdata);
        struct UnicodeString *astr=((struct Object *)a)->data, *bstr=((struct Object *)b)->data;
        if (astr->len != bstr->len)
                return 0;

        // memcmp is not reliable :( https://stackoverflow.com/a/11995514
        for (size_t i=0; i < astr->len; i++) {
                if (astr->val[i] != bstr->val[i])
                        return 0;
        }
        return 1;
}

struct Interpreter *interpreter_new(void)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		return NULL;

	interp->globalvars = hashtable_new(compare_string_objects);
	if (!(interp->globalvars)) {
		free(interp);
		return NULL;
	}
	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	hashtable_free(interp->globalvars);
	free(interp);
}
