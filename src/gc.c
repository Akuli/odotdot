#include "gc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "allobjects.h"
#include "objectsystem.h"
#include "objects/classobject.h"

// this thing works by setting gcflag to what refcount SHOULD be and then checking it
// Object.refcount and Object.gcflag are both ints

static void mark_reference(struct Object *referred, void *junkdata)
{
	referred->gcflag++;
}

void gc_run(struct Interpreter *interp)
{

	struct AllObjectsIter iter;
#define for_each_object iter = allobjects_iterbegin(interp->allobjects); while (allobjects_iternext(&iter))   // lol

	for_each_object
		((struct Object *) iter.obj)->gcflag = 0;

	for_each_object
		classobject_runforeachref((struct Object *) iter.obj, NULL, mark_reference);

	int problems = 0;
	for_each_object {
		struct Object *obj = iter.obj;
		if (obj->refcount != obj->gcflag) {
			if (!problems) {
				fprintf(stderr, "%s: reference counting problems\n", interp->argv0);
				problems=1;
			}
			fprintf(stderr, "   refcount of %p (class=%p) is %d too %s\n",
				(void*) obj, (void*) obj->klass, abs(obj->refcount - obj->gcflag),
				(obj->refcount > obj->gcflag ? "high" : "low"));
		}
	}

	// classes may be wiped before their instances
	// object_free_impl() uses ->klass unless ->klass is NULL
	for_each_object
		((struct Object *) iter.obj)->klass = NULL;

	// wipe everything, can't use for_each_object because interp->allobjects is modified in the loop
	for_each_object
		object_free_impl(interp, iter.obj, false);    // doesn't modify interp->allobjects because false

#undef for_each_object
}
