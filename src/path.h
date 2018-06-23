// utilities for working with path strings
#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
#define PATH_SLASH '\\'
#define PATH_SLASHSTR "\\"
#else
#define PATH_SLASH '/'
#define PATH_SLASHSTR "/"
#endif

// return current working directory as a \0-terminated string that must be free()'d
// sets errno and returns NULL on error
// if this runs out of mem, errno is set to ENOMEM and NULL is returned
// ENOMEM is not in C99, but windows and posix have it
char *path_getcwd(void);

// check if a path is absolute
// example: on non-Windows, /home/akuli/ö/src is absolute and ö/src is not
bool path_isabsolute(char *path);

// return a copy of path if it's absolute, otherwise path joined with current working directory
// return value must be free()'d
// returns NULL and sets errno on error (ENOMEM if malloc() fails)
char *path_toabsolute(char *path);

// join two paths by PATH_SLASH
// returns NULL on no mem
// return value must be free()'d
char *path_concat(char *path1, char *path2);

// on non-windows, path_find_last_slash("a/b/c.ö) returns the index of "/" before "c"
// on windows, same gibberish with backslashes
// useful for separating dirnames and basenames
size_t path_findlastslash(char *path);

#endif    // PATH_H
