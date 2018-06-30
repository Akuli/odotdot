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
char *import_findstd(char *argv0)
{
	char *cwd = path_getcwd();
	if (!cwd) {
		fprintf(stderr, "%s: cannot get current working directory: %s\n", argv0, strerror(errno));
		return NULL;
	}

	size_t len = strlen(cwd);
	assert(len >= 1);
	bool needslash = (cwd[len-1] != PATH_SLASH);

	void *tmp = realloc(cwd, strlen(cwd) + (size_t)needslash + sizeof("std") /* includes \0 */);
	if (!tmp) {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		free(cwd);
		return NULL;
	}
	cwd = tmp;

	strcat(cwd, needslash ? PATH_SLASHSTR"std" : "std");
	return cwd;
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

	void *tmp = realloc(filename, filenamelen+1);
	if (!tmp) {
		free(filename);
		return NULL;
	}
	filename = tmp;
	filename[filenamelen] = 0;
	assert(path_isabsolute(filename));

	size_t i = path_findlastslash(filename);
	char *srcdir = malloc(i+1);
	if (!srcdir) {
		errorobject_thrownomem(interp);
		free(filename);
		return NULL;
	}

	memcpy(srcdir, filename, i);
	free(filename);
	srcdir[i] = 0;
	return srcdir;
}


static unicode_char stdmarker[] = { '<','s','t','d','>' };

// returns a mallocced and \0-terminated string, throws an error and returns NULL on failure
static char *get_import_path(struct Interpreter *interp, struct UnicodeString name, struct Object *stackframe)
{
	struct UnicodeString ustdpath;
	if (!utf8_decode(interp, interp->stdpath, strlen(interp->stdpath), &ustdpath))
		return NULL;

#if PATH_SLASH != '/'
	unicode_char badslash = '/', goodslash = PATH_SLASH;
#endif

	// FIXME: unicodestring_replace() should take a mapping
	// this doesn't work if the replaced strings contain something to replace
	// in that case, the already replaced bits should never get replaced again
	struct UnicodeString replaces[][2] = {
#if PATH_SLASH != '/'
		{ (struct UnicodeString){ .len = 1, .val = &badslash }, (struct UnicodeString){ .len = 1, .val = &goodslash } },
#endif
		{ (struct UnicodeString){ .len = sizeof(stdmarker)/sizeof(unicode_char), .val = stdmarker }, ustdpath }
	};

	for (size_t i=0; i < sizeof(replaces)/sizeof(replaces[0]); i++) {
		struct UnicodeString *tmp = unicodestring_replace(interp, name, replaces[i][0], replaces[i][1]);
		if (i != 0) {    // the name came from unicodestring_replace, not from an argument
			// if name.len == 0, name.val can be NULL
			// but free(NULL) does nothing
			free(name.val);
		}

		if (!tmp) {
			free(ustdpath.val);
			return NULL;
		}
		name = *tmp;
		free(tmp);
	}
	free(ustdpath.val);

	char *fname;
	size_t fnamelen;   // without .ö or \0
	bool ok = utf8_encode(interp, name, &fname, &fnamelen);
	free(name.val);
	if (!ok)
		return NULL;

	void *tmp = realloc(fname, fnamelen + sizeof(".ö") /* includes \0 */);
	if (!tmp) {
		free(fname);
		return NULL;
	}
	fname = tmp;
	memcpy(fname+fnamelen, ".ö", sizeof(".ö"));

	char *fullpath;
	if (path_isabsolute(fname))
		fullpath = fname;
	else {
		char *sourcedir = get_source_dir(interp, stackframe);
		if (!sourcedir) {
			free(fname);
			return NULL;
		}
		fullpath = path_concat(sourcedir, fname);
		free(sourcedir);
		free(fname);
		if (!fullpath) {
			errorobject_thrownomem(interp);
			return NULL;
		}
	}

	return fullpath;
}

static struct Object *file_importer(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.String, interp->builtins.StackFrame, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct UnicodeString name = *((struct UnicodeString*) ARRAYOBJECT_GET(args, 0)->data);

	char *fullpath = get_import_path(interp, name, ARRAYOBJECT_GET(args, 1));
	if (!fullpath)
		return NULL;

	// check if the lib is cached
	struct Object *fullpathobj = stringobject_newfromcharptr(interp, fullpath);
	if (!fullpathobj) {
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
		free(fullpath);
	}
	if (status == 1)
		return cachedlib;
	if (status == -1)
		return NULL;
	assert(status == 0);

	// do what the built-in new function does when there's no ->newinstance
	assert(!((struct ClassObjectData*) interp->builtins.Library->data)->newinstance);
	struct Object *lib = object_new_noerr(interp, interp->builtins.Library, NULL, NULL, NULL);
	if (!lib) {
		errorobject_thrownomem(interp);
		OBJECT_DECREF(interp, fullpathobj);
		free(fullpath);
		return NULL;
	}

	status = run_libfile(interp, fullpath, lib);
	free(fullpath);
	if (status == -1) {    // file not found, try another importer
		OBJECT_DECREF(interp, lib);
		OBJECT_DECREF(interp, fullpathobj);
		return nullobject_get(interp);
	}
	if (status == 0) {   // error was thrown
		OBJECT_DECREF(interp, lib);
		OBJECT_DECREF(interp, fullpathobj);
		return NULL;
	}
	assert(status == 1);   // success

	bool ok = mappingobject_set(interp, interp->importstuff.filelibcache, fullpathobj, lib);
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
