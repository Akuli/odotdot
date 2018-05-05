// this is an implementation of "Naive mark and sweep"
// https://en.wikipedia.org/wiki/Tracing_garbage_collection#Na%C3%AFve_mark-and-sweep
#include "gc.h"
#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "dynamicarray.h"
#include "context.h"
#include "hashtable.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/classobject.h"

// run func(obj, data) for each object that interp references directly
// may call with same obj multiple times because the same object may be referred to in many places
static void foreach_baseset_element(struct Interpreter *interp, void (*func)(struct Object *, void *), void *data)
{
	func(interp->nomemerr->data, data);    // the message string
	func(interp->nomemerr, data);

	// TODO: support other contexts than interp->builtinctx
	struct HashTableIterator iter;
	hashtable_iterbegin(interp->builtinctx->localvars, &iter);
	while (hashtable_iternext(&iter)) {
		// TODO: how about everything that iter.value refers to??
		func(iter.value, data);
	}
}

static void mark(struct Object *obj, void *junkdata)
{
	if (obj->gcflag == 1)   // cyclic reference or called multiple times, see foreach_baseset_element()
		return;
	obj->gcflag = 1;

	struct ClassObjectData *classdata = obj->klass->data;
	if (classdata->foreachref)
		classdata->foreachref(obj, NULL, mark);
}


void gc_run(struct Interpreter *interp)
{
	// mark
	foreach_baseset_element(interp, mark, NULL);

	// sweep and unmark
	// can't sweep in the loop because object_free() mutates interp->allobjects
	struct DynamicArray *gonnasweep = dynamicarray_new();
	assert(gonnasweep);     // hope for the best

	struct HashTableIterator iter;
	hashtable_iterbegin(interp->allobjects, &iter);
	while (hashtable_iternext(&iter)) {
		if (((struct Object *) iter.key)->gcflag == 0)
			assert(dynamicarray_push(gonnasweep, iter.key) == STATUS_OK);    // hope for the best
		else
			((struct Object *) iter.key)->gcflag = 0;
	}

	// FIXME: do decref magics instead!
	while (gonnasweep->len != 0) {
		struct Object *obj = dynamicarray_pop(gonnasweep);
		obj->refcount = 0;
		object_free_impl(interp, obj);
	}
	dynamicarray_free(gonnasweep);
}
