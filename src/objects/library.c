#include "library.h"
#include <assert.h>
#include <stddef.h>
#include "classobject.h"
#include "../interpreter.h"

// the data of ArbitraryAttribs objects is NULL, the attributes are in attrdata (see objectsystem.h)
// so this is just a marker class that everything behaves weirdly with....
struct Object *libraryobject_createaaclass(struct Interpreter *interp)
{
	assert(interp->builtins.Object);
	return classobject_new(interp, "ArbitraryAttribs", interp->builtins.Object, NULL);
}

// this is also just a marker class... lol
struct Object *libraryobject_createclass(struct Interpreter *interp)
{
	assert(interp->builtins.ArbitraryAttribs);
	return classobject_new(interp, "Library", interp->builtins.ArbitraryAttribs, NULL);
}
