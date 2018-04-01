#include "errors.h"
#include <stdlib.h>
#include <string.h>
#include "../objectsystem.h"
#include "string.h"

static void error_foreachref(struct Object *obj, void *data, objectclassinfo_foreachrefcb cb)
{
	cb((struct Object *) obj->data, data);
}

struct ObjectClassInfo *errorobject_createclass(struct ObjectClassInfo *objectclass)
{
	return objectclassinfo_new(objectclass, error_foreachref, NULL);
}

// message string is created here because string constructors want to use interp->nomemerr and errptr
struct Object *errorobject_createnomemerr(struct ObjectClassInfo *errorclass, struct ObjectClassInfo *stringclass)
{
	struct UnicodeString *ustr = malloc(sizeof(struct UnicodeString));
	if (!ustr)
		return NULL;

	char msg[] = "not enough memory";
	ustr->len = strlen(msg);
	ustr->val = malloc(sizeof(uint32_t) * ustr->len);
	if (!(ustr->val)) {
		free(ustr);
		return NULL;
	}

	// can't use memcpy because different types
	for (size_t i=0; i < ustr->len; i++)
		ustr->val[i] = msg[i];

	struct Object *str = object_new(stringclass, ustr);
	if (!str) {
		free(ustr->val);
		free(ustr);
		return NULL;
	}

	struct Object *err = object_new(errorclass, str);
	if (!err) {
		object_free(err);   // takes care of ustr
		return NULL;
	}
	return err;
}
