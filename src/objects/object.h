#ifndef OBJECTS_OBJECT_H
#define OBJECTS_OBJECT_H

#include <stddef.h>
#include "../hashtable.h"


struct Object;   // forward declaration

// every ö class is represented as an ObjectClassInfo struct
struct ObjectClassInfo {
	// the baseclass is just for inspecting stuff, methods and attribute names
	// are copied from the baseclass when creating a new class
	// Object's baseclass is NULL
	struct ObjectClassInfo *baseclass;
	struct HashTable *methods;    // keys are AstStrInfos, values are Function objects
	struct HashTable *attrs;
	struct Object **methodnames;
	struct Object **methods;    // ö functions
	size_t nmethods;
	struct Object **attrnames;    // ö strings
	size_t nattrs;
};

struct Object {
	struct ObjectClassInfo *klass;
	struct Object **attrs;
};



#endif   // OBJECTS_OBJECT_H
