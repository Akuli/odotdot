#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "objectsystem.h"
#include "objects/classobject.h"

// this thing works by setting gcflag to what refcount SHOULD be and then checking it

static void mark_reference(struct Object *referred, void *junkdata)
{
	referred->gcflag++;
}

void gc_run(struct Interpreter *interp)
{
	// Object.refcount and Object.gcflag are both ints

	struct HashTableIterator iter;
	hashtable_iterbegin(interp->allobjects, &iter);
	while (hashtable_iternext(&iter))
		((struct Object *) iter.key)->gcflag = 0;

	hashtable_iterbegin(interp->allobjects, &iter);
	while (hashtable_iternext(&iter))
		classobject_runforeachref((struct Object *) iter.key, NULL, mark_reference);

	int problems = 0;
	hashtable_iterbegin(interp->allobjects, &iter);
	while (hashtable_iternext(&iter)) {
		struct Object *obj = iter.key;
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

	// wipe everything
	struct Object *obj;
	while (hashtable_getone(interp->allobjects, (void**)(&obj), NULL))
		object_free_impl(interp, obj, 0);
}
