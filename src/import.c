#include "import.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: make this less shitty
char *import_findstdlib(char *argv0)
{
	char *res = malloc(sizeof("stdlib"));
	if (!res) {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		return NULL;
	}

	memcpy(res, "stdlib", sizeof("stdlib"));
	return res;
}
