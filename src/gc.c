#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
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

	struct HashTableIterator iter;
#define for_each_object hashtable_iterbegin(interp->allobjects, &iter); while (hashtable_iternext(&iter))   // lol

	for_each_object
		((struct Object *) iter.key)->gcflag = 0;

	for_each_object
		classobject_runforeachref((struct Object *) iter.key, NULL, mark_reference);

	int problems = 0;
	for_each_object {
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

	// classes may be wiped before their instances
	// object_free_impl() uses ->klass unless ->klass is NULL
	for_each_object
		((struct Object *) iter.key)->klass = NULL;    // for object_free_impl()

	// wipe everything, can't use for_each_object because interp->allobjects is modified in the loop
	struct Object *obj;
	while (hashtable_getone(interp->allobjects, (void**)(&obj), NULL))
		object_free_impl(interp, obj);    // removes obj from interp->allobjects
}
