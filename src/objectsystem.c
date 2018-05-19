#include "objectsystem.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "hashtable.h"
#include "interpreter.h"
#include "unicode.h"
#include "objects/classobject.h"
#include "objects/mapping.h"

// the values of interp->allobjects are &dummy
static int dummy = 123;

struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*))
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->klass = klass;
	if (klass)
		OBJECT_INCREF(interp, klass);

	obj->attrs = NULL;
	obj->data = data;
	obj->hashable = 1;
	obj->hash = (unsigned int)((uintptr_t)obj);   // by default, hash by identity
	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;
	obj->destructor = destructor;

	if (hashtable_set(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummy, NULL) == STATUS_NOMEM) {
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

void object_free_impl(struct Interpreter *interp, struct Object *obj)
{
	// the refcount is > 0 if this is called from gc, so don't assert anything about that

	// obj->klass can be NULL, see builtins_setup() and gc_run()
	if (obj->klass)
		classobject_runforeachref(obj, (void*) interp, decref_the_ref);

	// this is called AFTER decreffing the refs, makes a difference for e.g. Array
	if (obj->destructor)
		obj->destructor(obj);

	assert(hashtable_pop(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), NULL, NULL) == 1);
	free(obj);
}
