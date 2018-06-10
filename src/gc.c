#include "gc.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "allobjects.h"
#include "objectsystem.h"
#include "objects/classobject.h"

// this thing works by setting gcflag to what refcount SHOULD be and then checking it
// Object.refcount and Object.gcflag are both longs

static void mark_reference(struct Object *referred, void *junkdata)
{
	referred->gcflag++;
}

#define PROBLEMS_MAX 500    // any more problems than this is an ACTUAL problem :D

void gc_run(struct Interpreter *interp)
{

	struct AllObjectsIter iter;
#define for_each_object iter = allobjects_iterbegin(interp->allobjects); while (allobjects_iternext(&iter))   // lol

	for_each_object
		((struct Object *) iter.obj)->gcflag = 0;

	for_each_object
		classobject_runforeachref((struct Object *) iter.obj, NULL, mark_reference);

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

	// classes may be wiped before their instances
	// object_free_impl() uses ->klass unless ->klass is NULL
	for_each_object
		((struct Object *) iter.obj)->klass = NULL;

	// wipe everything, can use for_each_object if interp->allobjects isn't modified in the loop
	for_each_object
		object_free_impl(interp, iter.obj, false);    // doesn't modify interp->allobjects because false

#undef for_each_object
}
