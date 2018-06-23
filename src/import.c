#include "import.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "objects/errors.h"
#include "objects/null.h"
#include "path.h"
#include "unicode.h"
#include "utf8.h"


#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#else
#undef WINDOWS
#endif



// TODO: make this not rely on cwd
char *import_findstdlib(char *argv0)
{
	char *cwd = path_getcwd();
	if (!cwd) {
		fprintf(stderr, "%s: cannot get current working directory: %s\n", argv0, strerror(errno));
		return NULL;
	}

	size_t len = strlen(cwd);
	assert(len >= 1);
	bool needslash = (cwd[len-1] != PATH_SLASH);

	void *tmp = realloc(cwd, strlen(cwd) + (size_t)needslash + sizeof("stdlib") /* includes \0 */);
	if (!tmp) {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		free(cwd);
		return NULL;
	}
	cwd = tmp;

	strcat(cwd, needslash ? PATH_SLASHSTR"stdlib" : "stdlib");
	return cwd;
}


static unicode_char stdlibmarker[] = { '<','s','t','d','l','i','b','>' };

struct Object *import(struct Interpreter *interp, struct UnicodeString name, char *sourcedir)
{
	struct UnicodeString ustdlibpath;
	if (!utf8_decode(interp, interp->stdlibpath, strlen(interp->stdlibpath), &ustdlibpath))
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
		{ (struct UnicodeString){ .len = sizeof(stdlibmarker)/sizeof(unicode_char), .val = stdlibmarker }, ustdlibpath }
	};

	for (size_t i=0; i < sizeof(replaces)/sizeof(replaces[0]); i++) {
		struct UnicodeString *tmp = unicodestring_replace(interp, name, replaces[i][0], replaces[i][1]);
		if (i != 0)    // the name came from unicodestring_replace
			free(name.val);

		if (!tmp) {
			free(ustdlibpath.val);
			return NULL;
		}
		name = *tmp;
		free(tmp);
	}
	free(ustdlibpath.val);

	char *fname_noext;
	size_t fname_noextlen;
	bool ok = utf8_encode(interp, name, &fname_noext, &fname_noextlen);
	free(name.val);
	if (!ok)
		return NULL;

	char fname[fname_noextlen + sizeof(".ö")];
	memcpy(fname, fname_noext, fname_noextlen);
	free(fname_noext);
	memcpy(fname+fname_noextlen, ".ö", sizeof(".ö"));

	char *fullpath;
	bool mustfreefullpath;
	if (path_isabsolute(fname)) {
		fullpath = fname;
		mustfreefullpath = false;
	} else {
		fullpath = path_concat(sourcedir, fname);
		if (!fullpath) {
			errorobject_thrownomem(interp);
			return NULL;
		}
		mustfreefullpath = true;
	}

	printf("** %s\n", fullpath);
	if (mustfreefullpath)
		free(fullpath);
	return nullobject_get(interp);
}

#undef UNICODE
