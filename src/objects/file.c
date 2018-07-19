#include "file.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../utf8.h"
#include "array.h"
#include "bool.h"
#include "bytearray.h"
#include "classobject.h"
#include "errors.h"
#include "integer.h"


/* usage:

	errno = 0;
	do something that may set errno;
	if (it failed) {
		THROW_FROM_ERRNO(interp, "IoError", "doing something failed");
		return something to indicate the failure;
	}

msg and fmt must be string literals

TODO: these should go to errors.h
*/
#define THROW_FROM_ERRNO(interp, classname, msg) do { \
	if (errno == 0) \
		errorobject_throwfmt((interp), (classname), msg); \
	else \
		errorobject_throwfmt((interp), (classname), msg": %s", strerror(errno)); \
} while (0)

#define THROW_FROM_ERRNO_FMT(interp, classname, fmt, ...) do { \
	if (errno == 0) \
		errorobject_throwfmt((interp), (classname), fmt": %s", __VA_ARGS__, strerror(errno)); \
	else \
		errorobject_throwfmt((interp), (classname), fmt, __VA_ARGS__); \
} while(0)


struct FileData {
	FILE *file;
	bool readable;
	bool writable;
	bool closed;
};

static void file_destructor(void *data)
{
	// no need to close the file here, my stdio(3) man page says that open files are closed on exit
	// ö code should close files, but not closing a file must not crash the interpreter even though it's bad
	free(data);
}


static struct Object *new_from_FILE(struct Interpreter *interp, struct Object *klass, FILE *f, bool reading, bool writing)
{
	struct FileData *fdata = malloc(sizeof *fdata);
	if (!fdata) {
		errorobject_thrownomem(interp);
		fclose(f);
		return NULL;
	}
	fdata->file = f;
	fdata->readable = reading;
	fdata->writable = writing;
	fdata->closed = false;

	struct Object *obj = object_new_noerr(interp, klass, (struct ObjectData){.data=fdata, .foreachref=NULL, .destructor=file_destructor});
	if (!obj)
	{
		errorobject_thrownomem(interp);
		free(fdata);
		fclose(f);
		return NULL;
	}
	return obj;
}

// this should be used only in io.ö
// args: path, binary, reading, writing
// TODO: allow writing so that the file is not overwritten, and the file may optionally be seeked to end [*]
static struct Object *newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.String, interp->builtins.Bool, interp->builtins.Bool, interp->builtins.Bool, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct Object *klass = ARRAYOBJECT_GET(args, 0);
	struct Object *pathobj = ARRAYOBJECT_GET(args, 1);
	bool binary = (ARRAYOBJECT_GET(args, 2) == interp->builtins.yes);
	bool reading = (ARRAYOBJECT_GET(args, 3) == interp->builtins.yes);
	bool writing = (ARRAYOBJECT_GET(args, 4) == interp->builtins.yes);
	assert(reading || writing);

	char *path;
	size_t pathlen;
	if (!utf8_encode(interp, *(struct UnicodeString*) pathobj->objdata.data, &path, &pathlen))
		return NULL;

	// add terminating \0
	void *tmp = realloc(path, pathlen+1);
	if (!tmp) {
		free(path);
		errorobject_thrownomem(interp);
		return NULL;
	}
	path = tmp;
	path[pathlen] = 0;

	char *mode;
	if (reading && writing)
		mode = binary? "wb+" : "w+";   // see [*] above
	else if (writing)
		mode = binary? "wb" : "w";
	else if (reading)
		mode = binary? "rb" : "r";
	else
		assert(0);    // like i commented... this is for io.ö only, and if this fails, io.ö screwed up

	errno = 0;
	FILE *f = fopen(path, mode);
	if (!f) {
		THROW_FROM_ERRNO_FMT(interp, "IoError", "cannot open %s", path);
		free(path);
		return NULL;
	}
	free(path);

	return new_from_FILE(interp, klass, f, reading, writing);
}

// newinstance does everything, this does nothing just to allow passing arguments handled by newinstance
static bool setup(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts) {
	return true;
}


static bool check_closed(struct Interpreter *interp, struct FileData fdata)
{
	if (fdata.closed) {
		errorobject_throwfmt(interp, "ValueError", "the file has been closed");
		return false;
	}
	return true;
}


static struct Object *closed_getter(struct Interpreter *interp, struct ObjectData nulldata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.File, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct FileData *fdata = ARRAYOBJECT_GET(args, 0)->objdata.data;
	return boolobject_get(interp, fdata->closed);
}

static struct Object *read_chunk(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Integer, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return NULL;
	if (!fdata->readable) {
		errorobject_throwfmt(interp, "ValueError", "the file is not readable", fileobj);
		return NULL;
	}

	long long tmp = integerobject_tolonglong(ARRAYOBJECT_GET(args, 0));
	if (tmp < 0) {
		errorobject_throwfmt(interp, "ValueError", "a negative size was passed to read_chunk");
		return NULL;
	}
	size_t maxsize = tmp;

	// fread returns 0 only on error unless the size is 0, then it also returns 0
	if (maxsize == 0)
		return bytearrayobject_new(interp, NULL, 0);

	unsigned char *buf = malloc(maxsize);
	if (!buf) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	errno = 0;
	size_t len = fread(buf, 1, maxsize, fdata->file);
	if (len == 0) {
		free(buf);
		if (feof(fdata->file))
			return bytearrayobject_new(interp, NULL, 0);
		THROW_FROM_ERRNO(interp, "IoError", "reading from file failed");
		return NULL;
	}
	return bytearrayobject_new(interp, buf, len);
}

static bool write_(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.ByteArray, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return false;
	if (!fdata->writable) {
		errorobject_throwfmt(interp, "IoError", "the file is not writable");
		return false;
	}

	struct Object *b = ARRAYOBJECT_GET(args, 0);

	// this doesn't need special casing when the bytearray is 0 bytes long
	if (fwrite(BYTEARRAYOBJECT_DATA(b), BYTEARRAYOBJECT_LEN(b), 1, fdata->file) != 1) {
		THROW_FROM_ERRNO(interp, "IoError", "writing to file failed");
		return false;
	}
	return true;
}

static bool set_pos(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Integer, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return false;

	long long offset = integerobject_tolonglong(ARRAYOBJECT_GET(args, 0));
	if (offset < 0)
		offset = 0;
	if (offset > LONG_MAX)
		offset = LONG_MAX;      // crazy! long is guaranteed to be at least 32 bits

	errno = 0;
	if (fseek(fdata->file, (long) offset, SEEK_SET) != 0) {
		THROW_FROM_ERRNO(interp, "IoError", "setting position of file failed");
		return false;
	}
	return true;
}

static struct Object *get_pos(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Integer, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return NULL;

	errno = 0;
	long result = ftell(fdata->file);
	if (result < 0) {
		THROW_FROM_ERRNO(interp, "IoError", "getting position of file failed");
		return NULL;
	}
	return integerobject_newfromlonglong(interp, result);
}

static bool flush(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return false;

	if (!fdata->writable) {
		// it seems like flushing read files may do something platform-specific but useful
		// not good for a cross-platform api
		errorobject_throwfmt(interp, "ValueError", "cannot flush non-writable files");
		return false;
	}

	errno = 0;
	if (fflush(fdata->file) != 0) {
		THROW_FROM_ERRNO(interp, "IoError", "flushing failed");
		return false;
	}
	return true;
}

static bool close_(struct Interpreter *interp, struct ObjectData thisdata, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, NULL)) return false;
	if (!check_no_opts(interp, opts)) return false;

	struct Object *fileobj = thisdata.data;
	struct FileData *fdata = fileobj->objdata.data;
	if (!check_closed(interp, *fdata))
		return false;

	errno = 0;
	if (fclose(fdata->file) != 0) {
		THROW_FROM_ERRNO(interp, "IoError", "closing failed");
		return false;
	}
	return true;
}


struct Object *fileobject_createclass(struct Interpreter *interp)
{
	// the Object baseclass will be replaced with another class in io.ö
	struct Object *klass = classobject_new(interp, "File", interp->builtins.Object, newinstance);
	if (!klass)
		return NULL;

	if (!attribute_add(interp, klass, "closed", closed_getter, NULL)) goto error;
	if (!method_add_noret(interp, klass, "setup", setup)) goto error;
	if (!method_add_yesret(interp, klass, "read_chunk", read_chunk)) goto error;
	if (!method_add_noret(interp, klass, "write", write_)) goto error;
	if (!method_add_noret(interp, klass, "set_pos", set_pos)) goto error;
	if (!method_add_yesret(interp, klass, "get_pos", get_pos)) goto error;
	if (!method_add_noret(interp, klass, "flush", flush)) goto error;
	if (!method_add_noret(interp, klass, "close", close_)) goto error;
	return klass;

error:
	OBJECT_DECREF(interp, klass);
	return NULL;
}
