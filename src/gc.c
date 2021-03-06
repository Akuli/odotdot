#include "gc.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "allobjects.h"
#include "interpreter.h"
#include "objectsystem.h"

// this thing works by setting gcflag to what refcount SHOULD be and then checking it
// Object.refcount and Object.gcflag are both longs

static void mark_reference(struct Object *ref, void *junkdata)
{
	ref->gcflag++;
}

#define PROBLEMS_MAX 500    // any more problems than this is an ACTUAL problem :D

void gc_run(struct Interpreter *interp)
{

	struct AllObjectsIter iter;
#define for_each_object iter = allobjects_iterbegin(interp->allobjects); while (allobjects_iternext(&iter))   // lol

	for_each_object
		iter.obj->gcflag = 0;

	for_each_object {
		if (iter.obj->klass)
			iter.obj->klass->gcflag++;
		if (iter.obj->attrdata)
			iter.obj->attrdata->gcflag++;
		if (iter.obj->objdata.foreachref)
			iter.obj->objdata.foreachref(iter.obj->objdata.data, mark_reference, NULL);
	}

	bool gotproblems = false;
	struct Object *problems[PROBLEMS_MAX] = { NULL };  // for debugging
	struct Object **problemptr = problems;

	for_each_object {
		struct Object *obj = iter.obj;
		if (obj->refcount != obj->gcflag) {
			if (!gotproblems) {
				fprintf(stderr, "%s: reference counting problems\n", interp->argv0);
				gotproblems=true;
			}
			fprintf(stderr, "   refcount of %p (class=%p) is %d too %s\n",
				(void*) obj, (void*) obj->klass, abs(obj->refcount - obj->gcflag),
				(obj->refcount > obj->gcflag ? "high" : "low"));
			*problemptr++ = obj;
		}
	}

	// fail here in a debuggable way if there are problems
	// i can gdb this and then look at the problems array
	assert(!gotproblems);

	// wipe everything, can use for_each_object if interp->allobjects isn't modified in the loop
	for_each_object
		object_free_impl(interp, iter.obj, true);    // doesn't modify interp->allobjects because the true arg

#undef for_each_object
}
