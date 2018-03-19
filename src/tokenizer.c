// TODO: use wchars?

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dynamicarray.h"
#include "tokenizer.h"

#define CHUNK_SIZE 4096


char *read_file_to_huge_string(char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		goto error_noclose;

	char *s = NULL;
	char buf[CHUNK_SIZE];
	size_t totalsize=0;
	size_t nread;
	while ((nread = fread(buf, 1, CHUNK_SIZE, f))) {
		// <jp> if s is NULL in realloc(s, sz), it acts like malloc
		s = realloc(s, totalsize+nread+1);
		if (!s)
			goto error;
		memcpy(s+totalsize, buf, nread);
		totalsize += nread;
	}
	s[totalsize] = 0;

	if (!feof(f))
		goto error;
	if (fclose(f) == EOF)
		// don't try to close it again, that would be undefined behaviour
		goto error_noclose;

	return s;

error:
	fclose(f);
	// "fall through" to error_noclose
error_noclose:
	if (s)
		free(s);
	return NULL;
}


static struct Token *new_token(char kind, char *huge_string, size_t val_stringlen, size_t lineno)
{
	struct Token *tok = malloc(sizeof (struct Token));
	tok->kind = kind;
	tok->val_firstchar = huge_string[0];
	tok->lineno = lineno;

	tok->val = malloc(val_stringlen+1);
	if (!(tok->val))
		return NULL;
	for (size_t i=0; i < val_stringlen; ++i)
		tok->val[i] = huge_string[i];
	tok->val[val_stringlen] = 0;

	return tok;
}


void token_free(struct Token *tok)
{
	free(tok->val);
	free(tok);
}


// because c standards
static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }

struct DynamicArray *token_ize(char *huge_string)
{
	struct DynamicArray *arr = dynamicarray_new();
	if (!arr)
		return NULL;
	size_t lineno=1;

	struct Token *tok;
	char kind;
	size_t stringlen;

	while (huge_string[0]) {
		if (isspace(huge_string[0])) {
			if (huge_string[0] == '\n')
				lineno++;
			huge_string++;
			continue;
		}

		if (huge_string[0] == '#') {
			while (huge_string[0] != 0 && huge_string[0] != '\n')
				huge_string++;
			continue;
		}

#define f(x) (huge_string[0]==(x))
		if (f('{')||f('}')||f('[')||f(']')||f('(')||f(')')||f('=')||f(';')||f('.')) {
#undef f
			kind = TOKENKIND_OP;
			stringlen = 1;
		}

		else if (huge_string[0] == 'v' &&
				huge_string[1] == 'a' &&
				huge_string[2] == 'r' &&
				(huge_string[3] && !isalnum(huge_string[3]))) {
			kind = TOKENKIND_KEYWORD;
			stringlen = 3;
		}

		else if (huge_string[0] == '"') {
			kind = TOKENKIND_STRING;
			stringlen = 1;
			while (huge_string[stringlen] != '"') {
				assert(huge_string[stringlen]);     // TODO: handle unexpected EOFs better
				assert(huge_string[stringlen] != '\n');   // TODO: handle unexpected newlines better
				stringlen++;
			}
			stringlen++;     // last "
		}

		else if (isdigit(huge_string[0])) {
			kind = TOKENKIND_INTEGER;
			stringlen = 0;
			while (isdigit(huge_string[stringlen]))
				stringlen++;
		}

		else if (huge_string[0] == '-' && isdigit(huge_string[1])) {
			kind = TOKENKIND_INTEGER;
			stringlen = 1;
			while (isdigit(huge_string[stringlen]))
				stringlen++;
		}

		else if (isalpha(huge_string[0])) {
			kind = TOKENKIND_ID;
			stringlen = 0;
			while (huge_string[stringlen] && isalnum(huge_string[stringlen]))
				stringlen++;
		}

		else {
			fprintf(stderr, "I don't know how to tokenize '%c' (line %llu)\n",
				huge_string[0], (unsigned long long)lineno);
			goto error;
		}

		tok = new_token(kind, huge_string, stringlen, lineno);
		if (!tok)
			goto error;
		huge_string += stringlen;    // must be after new_token()
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
