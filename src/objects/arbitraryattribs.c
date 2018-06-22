#include "arbitraryattribs.h"
#include <assert.h>
#include "classobject.h"

// the data of ArbitraryAttribs objects is NULL, the attributes are in attrdata (see objectsystem.h)
// so this is just a marker class that everything behaves weirdly with....
struct Object *arbitraryattribsobject_createclass(struct Interpreter *interp)
{
	assert(interp->builtins.Object);
	return classobject_new(interp, "ArbitraryAttribs", interp->builtins.Object, NULL);
}
