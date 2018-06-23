#include "path.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#include <direct.h>
#else
#undef WINDOWS
#include <unistd.h>
#endif


// this is one of the not-so-many things that windows makes easy... :D
char *path_getcwd(void)
{
#ifdef WINDOWS
	// https://msdn.microsoft.com/en-us/library/sf98bd4y.aspx
	// "The buffer argument can be NULL; a buffer of at least size maxlen (more only
	//  if necessary) is automatically allocated, using malloc, to store the path."
	// and in the "Return Value" section:
	// "A NULL return value indicates an error, and errno is set either to ENOMEM,
	//  indicating that there is insufficient memory to allocate maxlen bytes (when
	//  a NULL argument is given as buffer), or [...]"
	return _getcwd(NULL, 1);

#else
	// unixy getcwd() wants a buffer of fixed size
	char *buf = NULL;
	for (size_t bufsize = 64; ; bufsize *= 2) {
		void *tmp = realloc(buf, bufsize);     // mallocs if buf is NULL
		if (!tmp) {
			errno = ENOMEM;
			return NULL;
		}
		buf = tmp;

		if (!getcwd(buf, bufsize)) {
			if (errno == ERANGE)     // buffer was too small
				continue;
			return NULL;
		}

		// got the cwd, need to remove trailing slashes
		size_t len = strlen(buf);
		while (buf[len-1] == PATH_SLASH && len > 0)
			len--;
		return buf;
	}

#endif
}

bool path_isabsolute(char *path)
{
#if WINDOWS
	// "\asd\toot" is equivalent to "X:\asd\toot" where X is... the drive of cwd i guess?
	// path[0] is '\0' if the path is empty
	if (path[0] == '\\')
		return true;

	// check for X:\asdasd
	return (('A' <= path[0] && path[0] <= 'Z') || ('a' <= path[0] && path[0] <= 'z')) &&
		path[1] == ':' && path[2] == '\\';

#else
	return path[0]=='/';

#endif
}

char *path_toabsolute(char *path)
{
	if (path_isabsolute(path)) {
		size_t len = strlen(path);
		char *res = malloc(len+1);
		if (!res) {
			errno = ENOMEM;
			return NULL;
		}
		memcpy(res, path, len+1);
		return res;
	}

	char *cwd = path_getcwd();
	if (!cwd)
		return NULL;

	char *res = path_concat(cwd, path);
	free(cwd);
	if (!res)
		errno = ENOMEM;
	return res;
}

char *path_concat(char *path1, char *path2)
{
	size_t len1 = strlen(path1);
	size_t len2 = strlen(path2);

	if (len1 == 0) {
		char *res = malloc(len2+1);
		if (!res)
			return NULL;
		memcpy(res, path2, len2 + 1);
		return res;
	}
	// python returns 'asd/' for os.path.join('asd', ''), maybe that's good

	bool needslash = (path1[len1-1] != PATH_SLASH);
	char *res = malloc(len1 + (size_t)needslash + len2 + 1 /* for \0 */);
	if (!res)
		return NULL;

	memcpy(res, path1, len1);
	if (needslash) {
		res[len1] = PATH_SLASH;
		memcpy(res+len1+1, path2, len2+1);
	} else
		memcpy(res+len1, path2, len2+1);
	return res;
}

size_t path_findlastslash(char *path)
{
	if (path[0] == 0)
		return 0;

	// if the path ends with a slash, it must be ignored
	// this is also the reason why strchr() isn't useful here
	size_t i = strlen(path)-1;
	while (i >= 1 && path[i] == PATH_SLASH)
		i--;

	// TODO: i think C:asd.ö is a valid windows path??
	for (; i >= 1; i-- /* behaves well because >=1 */)
	{
		if (path[i] == PATH_SLASH)
			return i;
	}
	return 0;
}
