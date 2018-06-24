#include "import.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "attribute.h"
#include "check.h"
#include "interpreter.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/mapping.h"
#include "objects/null.h"
#include "objects/string.h"
#include "objectsystem.h"
#include "path.h"
#include "run.h"
#include "unicode.h"
#include "utf8.h"

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#else
#undef WINDOWS
#endif


// TODO: make this not rely on cwd
char *import_findstdlibs(char *argv0)
{
	char *cwd = path_getcwd();
	if (!cwd) {
		fprintf(stderr, "%s: cannot get current working directory: %s\n", argv0, strerror(errno));
		return NULL;
	}

	size_t len = strlen(cwd);
	assert(len >= 1);
	bool needslash = (cwd[len-1] != PATH_SLASH);

	void *tmp = realloc(cwd, strlen(cwd) + (size_t)needslash + sizeof("stdlibs") /* includes \0 */);
	if (!tmp) {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		free(cwd);
		return NULL;
	}
	cwd = tmp;

	strcat(cwd, needslash ? PATH_SLASHSTR"stdlibs" : "stdlibs");
	return cwd;
}


struct Object *import(struct Interpreter *interp, struct Object *namestring, struct Object *stackframe)
{
	assert(interp->importstuff.importers);
	assert(interp->builtins.Function);

	for (size_t i=0; i < ARRAYOBJECT_LEN(interp->importstuff.importers); i++)
	{
		struct Object *importer = ARRAYOBJECT_GET(interp->importstuff.importers, i);
		if (!check_type(interp, interp->builtins.Function, importer))
			return NULL;

		struct Object *lib = functionobject_call(interp, importer, namestring, stackframe, NULL);
		if (!lib)
			return NULL;
		if (lib == interp->builtins.null) {
			OBJECT_DECREF(interp, lib);
			continue;
		}
		if (!classobject_isinstanceof(lib, interp->builtins.Library)) {
			errorobject_throwfmt(interp, "TypeError", "importer functions should return a Library object or null, but %D returned %D", importer, lib);
			OBJECT_DECREF(interp, lib);
			return NULL;
		}
		return lib;
	}

	// FIXME: ValueError feels wrong
	errorobject_throwfmt(interp, "ValueError", "cannot import %D", namestring);
	return NULL;
}


static char *get_source_dir(struct Interpreter *interp, struct Object *stackframe)
{
	struct Object *filenameobj = attribute_get(interp, stackframe, "filename");
	if (!filenameobj)
		return NULL;
	if (!check_type(interp, interp->builtins.String, filenameobj)) {
		OBJECT_DECREF(interp, filenameobj);
		return NULL;
	}

	char *filename;
	size_t filenamelen;
	bool ok = utf8_encode(interp, *((struct UnicodeString*) filenameobj->data), &filename, &filenamelen);
	OBJECT_DECREF(interp, filenameobj);
	if (!ok)
		return NULL;

	char filename0[filenamelen+1];
	memcpy(filename0, filename, filenamelen);
	free(filename);
	filename0[filenamelen] = 0;
	assert(path_isabsolute(filename0));

	size_t i = path_findlastslash(filename0);
	char *srcdir = malloc(i+1);
	if (!srcdir) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	memcpy(srcdir, filename0, i);
	srcdir[i] = 0;
	return srcdir;
}

static unicode_char stdlibsmarker[] = { '<','s','t','d','l','i','b','s','>' };

static struct Object *file_importer(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.StackFrame, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct UnicodeString name = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data);

	struct UnicodeString ustdlibspath;
	if (!utf8_decode(interp, interp->stdlibspath, strlen(interp->stdlibspath), &ustdlibspath))
		return NULL;

#if PATH_SLASH != '/'
	unicode_char badslash = '/', goodslash = PATH_SLASH;
#endif

	// TODO: check what happens with an empty name
	// FIXME: unicodestring_replace() should take a mapping
	// this doesn't work if the replaced strings contain something to replace
	// in that case, the already replaced bits should never get replaced again
	struct UnicodeString replaces[][2] = {
#if PATH_SLASH != '/'
		{ (struct UnicodeString){ .len = 1, .val = &badslash }, (struct UnicodeString){ .len = 1, .val = &goodslash } },
#endif
		{ (struct UnicodeString){ .len = sizeof(stdlibsmarker)/sizeof(unicode_char), .val = stdlibsmarker }, ustdlibspath }
	};

	for (size_t i=0; i < sizeof(replaces)/sizeof(replaces[0]); i++) {
		struct UnicodeString *tmp = unicodestring_replace(interp, name, replaces[i][0], replaces[i][1]);
		if (i != 0)    // the name came from unicodestring_replace
			free(name.val);

		if (!tmp) {
			free(ustdlibspath.val);
			return NULL;
		}
		name = *tmp;
		free(tmp);
	}
	free(ustdlibspath.val);

	char *fname_noext;
	size_t fname_noextlen;
	bool ok = utf8_encode(interp, name, &fname_noext, &fname_noextlen);
	free(name.val);
	if (!ok)
		return NULL;

	char fname[fname_noextlen + sizeof(".ö") /* includes \0 */];
	memcpy(fname, fname_noext, fname_noextlen);
	free(fname_noext);
	memcpy(fname+fname_noextlen, ".ö", sizeof(".ö"));

	char *fullpath;
	bool mustfreefullpath;
	if (path_isabsolute(fname)) {
		fullpath = fname;
		mustfreefullpath = false;
	} else {
		char *sourcedir = get_source_dir(interp, ARRAYOBJECT_GET(args, 1));
		if (!sourcedir)
			return NULL;
		fullpath = path_concat(sourcedir, fname);
		free(sourcedir);
		if (!fullpath) {
			errorobject_thrownomem(interp);
			return NULL;
		}
		mustfreefullpath = true;
	}

	// check if the lib is cached
	struct Object *fullpathobj = stringobject_newfromcharptr(interp, fullpath);
	if (!fullpathobj) {
		if (mustfreefullpath)
			free(fullpath);
		return NULL;
	}

	// TODO: lowercase fullpathobj on case-insensitive file systems?
	// "case-insensitive file systems" is not same as windows, e.g. osx is case-insensitive
	// unless you install it in an unusual way or something... anyway
	assert(interp->importstuff.filelibcache);
	struct Object *cachedlib;
	int status = mappingobject_get(interp, interp->importstuff.filelibcache, fullpathobj, &cachedlib);
	if (status != 0) {   // gonna return soon
		OBJECT_DECREF(interp, fullpathobj);
		if (mustfreefullpath)
			free(fullpath);
	}
	if (status == 1)
		return cachedlib;
	if (status == -1)
		return NULL;
	assert(status == 0);

	// TODO: check file not found error
	struct Object *vars;
	status = run_libfile(interp, fullpath, &vars);
	if (mustfreefullpath)
		free(fullpath);
	if (status == -1) {    // file not found, try another importer
		OBJECT_DECREF(interp, fullpathobj);
		return nullobject_get(interp);
	}
	if (status == 0) {   // error was thrown
		OBJECT_DECREF(interp, fullpathobj);
		return NULL;
	}
	assert(status == 1);   // success

	// do what the built-in new function does when there's no ->newinstance
	assert(!((struct ClassObjectData*) interp->builtins.Library->data)->newinstance);
	struct Object *lib = object_new_noerr(interp, interp->builtins.Library, NULL, NULL, NULL);
	if (!lib) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, vars);
		OBJECT_DECREF(interp, fullpathobj);
		return NULL;
	}

	struct MappingObjectIter iter;
	mappingobject_iterbegin(&iter, vars);
	while (mappingobject_iternext(&iter)) {
		if (!attribute_setwithstringobj(interp, lib, iter.key, iter.value)) {
			OBJECT_DECREF(interp, lib);
			OBJECT_DECREF(interp, vars);
			OBJECT_DECREF(interp, fullpathobj);
			return NULL;
		}
	}
	OBJECT_DECREF(interp, vars);

	ok = mappingobject_set(interp, interp->importstuff.filelibcache, fullpathobj, lib);
	OBJECT_DECREF(interp, fullpathobj);
	if (!ok) {
		OBJECT_DECREF(interp, lib);
		return NULL;
	}

	return lib;
}


bool import_init(struct Interpreter *interp)
{
	assert(interp->importstuff.importers);

	struct Object *fileimp = functionobject_new(interp, file_importer, "file_importer");
	if (!fileimp)
		return NULL;
	bool ok = arrayobject_push(interp, interp->importstuff.importers, fileimp);
	OBJECT_DECREF(interp, fileimp);
	return ok;
}
