// TODO: use wchars?

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamicarray.h"
#include "utf8.h"
#include "tokenizer.h"

#define CHUNK_SIZE 4096


int read_file_to_huge_string(FILE *f, char **dest, size_t *destlen)
{
	int errorcode=0;

	char *s = NULL;
	char buf[CHUNK_SIZE];
	size_t totalsize=0;
	size_t nread;
	while ((nread = fread(buf, 1, CHUNK_SIZE, f))) {
		// <jp> if s is NULL in realloc(s, sz), it acts like malloc
		char *ptr = realloc(s, totalsize+nread);
		if (!ptr) {
			errorcode = -1;
			goto error;
		}
		s = ptr;
		memcpy(s+totalsize, buf, nread);
		totalsize += nread;
	}

	if (!feof(f)) {
		errorcode = 1;
		goto error;
	}

	*dest = s;
	*destlen = totalsize;
	return 0;

error:
	if (s)
		free(s);
	return errorcode;
}


static struct Token *new_token(char kind, unsigned long *val, size_t vallen, size_t lineno)
{
	struct Token *tok = malloc(sizeof (struct Token));
	if (!tok)
		return NULL;
	tok->val = malloc(vallen * sizeof(unsigned long));
	if (!(tok->val)) {
		free(tok);
		return NULL;
	}

	tok->kind = kind;
	memcpy(tok->val, val, vallen*sizeof(unsigned long));
	tok->vallen = vallen;
	tok->lineno = lineno;
	return tok;
}


void token_free(struct Token *tok)
{
	free(tok->val);
	free(tok);
}


// because c standards
static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }


// TODO: better error handling than fprintf
// TODO: test the error cases :(
struct DynamicArray *token_ize(unsigned long *hugestring, size_t hugestringlen)
{
	struct DynamicArray *arr = dynamicarray_new();
	if (!arr)
		return NULL;
	size_t lineno=1;

	struct Token *tok;
	char kind;
	size_t nchars;    // comparing size_t with size_t produces no warnings

	while (hugestringlen) {
		// TODO: check for any unicode whitespace
#define f(x) (hugestring[0]==(unsigned long)(x))
		if (f(' ')||f('\t')||f('\n')) {
			if (f('\n'))
				lineno++;
			hugestring++;
			hugestringlen--;
			continue;
		}

		if (hugestring[0] == '#') {
			while (hugestringlen && hugestring[0] != '\n') {
				hugestring++;
				hugestringlen--;
			}
			continue;
		}

		if (f('{')||f('}')||f('[')||f(']')||f('(')||f(')')||f('=')||f(';')||f('.')) {
#undef f
			kind = TOKENKIND_OP;
			nchars = 1;
		}

		else if (hugestringlen >= 4 &&
				hugestring[0] == (unsigned long)'v' &&
				hugestring[1] == (unsigned long)'a' &&
				hugestring[2] == (unsigned long)'r' &&
				(hugestring[3] && !unicode_isalnum(hugestring[3]))) {
			kind = TOKENKIND_KEYWORD;
			nchars = 3;
		}

		else if (hugestring[0] == '"') {
			kind = TOKENKIND_STRING;
			nchars = 1;    // first "
			while ((hugestringlen > nchars) &&
					(hugestring[nchars] != (unsigned long)'"')) {
				if (hugestring[nchars] == (unsigned long)'\n') {
					fprintf(stderr, "line %llu: ending \" must be on the same line as starting \"", (unsigned long long)lineno);
					goto error;
				}
				nchars++;
			}
			nchars++;    // last ", the error handling stuff below runs if this is missing
		}

		else if (unicode_is0to9(hugestring[0])) {
			kind = TOKENKIND_INTEGER;
			nchars = 0;
			while (hugestringlen > nchars && unicode_is0to9(hugestring[nchars]))
				nchars++;
		}

		else if (hugestringlen >= 2 && hugestring[0] == '-' && unicode_is0to9(hugestring[1])) {
			kind = TOKENKIND_INTEGER;
			nchars = 1;
			while (hugestringlen > nchars && unicode_is0to9(hugestring[nchars]))
				nchars++;
		}

		else if (unicode_isalpha(hugestring[0])) {
			kind = TOKENKIND_ID;
			nchars = 0;
			while (hugestringlen > nchars && unicode_isalnum(hugestring[nchars]))
				nchars++;
		}

		else {
			fprintf(stderr, "line %llu: unknown token\n", (unsigned long long)lineno);
			goto error;
		}

		if (hugestringlen < nchars) {
			fprintf(stderr, "unexpected end of file\n");
			goto error;
		}

		tok = new_token(kind, hugestring, nchars, lineno);
		if (!tok)
			goto error;
		hugestring += nchars;    // must be after new_token()
		hugestringlen -= nchars;
		if (dynamicarray_push(arr, (void *)tok)) {
			free(tok);
			goto error;
		}
	}
	return arr;

error:
	dynamicarray_freeall(arr, token_free_voidstar);
	return NULL;
}
