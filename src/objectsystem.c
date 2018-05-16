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

// the values of interp->allobjects are &dummy
static int dummy = 123;

static int compare_unicode_strings(void *a, void *b, void *userdata)
{
	assert(!userdata);
	struct UnicodeString *astr=a, *bstr=b;
	if (astr->len != bstr->len)
		return 0;

	// memcmp is not reliable :( https://stackoverflow.com/a/11995514
	for (size_t i=0; i < astr->len; i++) {
		if (astr->val[i] != bstr->val[i])
			return 0;
	}
	return 1;
}

struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data, void (*destructor)(struct Object*))
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	// if klass is NULL, DON'T create attributes
	// see builtins_setup() for the classes that this happens to
	if (klass && ((struct ClassObjectData *) klass->data)->instanceshaveattrs) {
		obj->attrs = hashtable_new(compare_unicode_strings);
		if (!obj->attrs) {
			free(obj);
			return NULL;
		}
	} else {
		obj->attrs = NULL;
	}

	obj->klass = klass;
	if (klass)
		OBJECT_INCREF(interp, klass);

	obj->data = data;
	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;
	obj->destructor = destructor;

	if (hashtable_set(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummy, NULL) == STATUS_NOMEM) {
		if (klass)
			OBJECT_DECREF(interp, klass);
		if (obj->attrs)
			hashtable_free(obj->attrs);
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

	if (obj->destructor)
		obj->destructor(obj);

	assert(hashtable_pop(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), NULL, NULL) == 1);

	if (obj->attrs) {
		// the attributes should have been decreffed already if needed, see Object's foreachref
		hashtable_clear(obj->attrs);
		hashtable_free(obj->attrs);
	}
	free(obj);
}
