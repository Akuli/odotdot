#include "objectsystem.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "interpreter.h"
#include "unicode.h"
#include "objects/classobject.h"
#include "objects/mapping.h"

struct Object *object_new_noerr(struct Interpreter *interp, struct Object *klass,
	void *data, void (*foreachref)(struct Object *, void *, object_foreachrefcb), void (*destructor)(struct Object *))
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->klass = klass;
	if (klass)
		OBJECT_INCREF(interp, klass);

	obj->data = data;
	obj->foreachref = foreachref;
	obj->destructor = destructor;

	obj->attrdata = NULL;

	// the rightmost bits are often used for alignment
	// let's throw them away for a better distribution
	obj->hash = (uintptr_t)((void*)obj) >> 2;
	obj->hashable = true;

	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;

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
	OBJECT_DECREF((struct Interpreter *)interpdata, ref);
}

void object_free_impl(struct Interpreter *interp, struct Object *obj, bool calledfromgc)
{
	// the refcount is > 0 if this is called from gc.c, so don't assert anything about that

	if (!calledfromgc) {
		// decref other objects that obj references to
		// gc_run() takes care of calling this for each object
		if (obj->attrdata)
			OBJECT_DECREF(interp, obj->attrdata);
		if (obj->foreachref)
			obj->foreachref(obj, (void*)interp, decref_the_ref);
		if (obj->klass)   // can be NULL, see builtins_setup()
			OBJECT_DECREF(interp, obj->klass);
	}

	// this is called AFTER decreffing the refs, makes a difference for e.g. Array
	if (obj->destructor)
		obj->destructor(obj);

	if (!calledfromgc)
		assert(allobjects_remove(&(interp->allobjects), obj));
	free(obj);
}
