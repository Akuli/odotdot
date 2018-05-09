#include "objectsystem.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

struct Object *object_new(struct Interpreter *interp, struct Object *klass, void *data)
{
	struct Object *obj = malloc(sizeof(struct Object));
	if(!obj)
		return NULL;

	obj->attrs = hashtable_new(compare_unicode_strings);
	if (!obj->attrs) {
		free(obj);
		return NULL;
	}

	obj->klass = klass;
	if (klass)
		OBJECT_INCREF(interp, klass);

	obj->data = data;
	obj->refcount = 1;   // the returned reference
	obj->gcflag = 0;

	if (hashtable_set(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummy, NULL) == STATUS_NOMEM) {
		if (klass)
			OBJECT_DECREF(interp, klass);
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
	// these two asserts make sure that the sign of the refcount is shown in error messages
	assert(obj->refcount >= 0);
	assert(obj->refcount <= 0);

	// obj->klass can be NULL, see builtins_teardown()
	if (obj->klass) {
		OBJECT_INCREF(interp, obj->klass);   // temporary, decreffed after destructor call

		struct Object *curclass = obj->klass;
		struct ClassObjectData *curclassdata;
		do {
			curclassdata = curclass->data;
			if (curclassdata->foreachref)
				curclassdata->foreachref(obj, interp, decref_the_ref);
		} while ((curclass = curclassdata->baseclass));

		struct ClassObjectData *classdata = obj->klass->data;
		if (classdata->destructor)
			classdata->destructor(obj);   // may need obj->klass
		OBJECT_DECREF(interp, obj->klass);
	}

	void *dummyptr;
	assert(hashtable_pop(interp->allobjects, obj, (unsigned int)((uintptr_t)obj), &dummyptr, NULL) == 1);

	hashtable_clear(obj->attrs);   // TODO: decref the values or something?
	hashtable_free(obj->attrs);
	free(obj);
}
