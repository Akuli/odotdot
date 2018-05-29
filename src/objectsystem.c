#include "objectsystem.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "interpreter.h"
#include "unicode.h"
#include "objects/classobject.h"
#include "objects/mapping.h"

struct Object *object_new_noerr(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*))
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->klass = klass;
	if (klass)
		OBJECT_INCREF(interp, klass);

	obj->attrs = NULL;
	obj->data = data;

	// the rightmost bits are often used for alignment
	// let's throw them away for a better distribution
	obj->hash = (uintptr_t)((void*)obj) >> 3;
	obj->hashable = 1;

	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;
	obj->destructor = destructor;

	if (!allobjects_add(&(interp->allobjects), obj)) {
		if (klass)
			OBJECT_DECREF(interp, klass);
		free(obj);
		return NULL;
	}

	return obj;
}

static void decref_the_ref(struct Object *ref, void *interpdata)
{
	OBJECT_DECREF(interpdata, ref);
}

void object_free_impl(struct Interpreter *interp, struct Object *obj, bool deletefromallobjects)
{
	// the refcount is > 0 if this is called from gc, so don't assert anything about that

	// obj->klass can be NULL, see builtins_setup() and gc_run()
	if (obj->klass)
		classobject_runforeachref(obj, (void*) interp, decref_the_ref);

	// this is called AFTER decreffing the refs, makes a difference for e.g. Array
	if (obj->destructor)
		obj->destructor(obj);

	if (deletefromallobjects)
		assert(allobjects_remove(&(interp->allobjects), obj));
	free(obj);
}
