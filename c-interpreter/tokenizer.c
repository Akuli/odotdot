// TODO: use wchars?

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dynamicarray.h"
#include "tokenizer.h"

unsigned char *read_file_to_huge_string(unsigned char *path)
{
	FILE *f = NULL;

	struct DynamicArray *arr = dynamicarray_new();
	if (!arr)
		return NULL;

	// FIXME: this cast is :/
	f = fopen((char *)path, "r");
	if (!f)
		goto error;

	// TODO: don't read 1 byte at a time :(
	char c;
	while (fread(&c, 1, 1, f) == 1) {
		unsigned char *c2 = malloc(1);
		*c2 = c;
		dynamicarray_push(arr, c2);
	}

	if (!feof(f))
		goto error;
	if (fclose(f) == EOF)
		goto error_noclose;

	unsigned char *s = malloc(arr->len + 1);
	if (!s)
		goto error_noclose;

	for (size_t i=0; i < arr->len; i++) {
		s[i] = *((unsigned char *) arr->values[i]);
	}
	s[arr->len] = 0;

	while (arr->len)
		free(dynamicarray_pop(arr));
	dynamicarray_free(arr);

	return s;

error:
	if (f)
		fclose(f);
	// "fall through" to error_noclose
error_noclose:
	dynamicarray_free2(arr, free);
	return NULL;
}


static struct Token *new_token(char kind, unsigned char *huge_string, size_t val_stringlen, size_t lineno)
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


struct DynamicArray *token_ize(unsigned char *huge_string)
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
			fprintf(stderr, "I don't know how to tokenize '%c' (line %lu)\n", huge_string[0], (unsigned long)lineno);
			goto error;
		}

		tok = new_token(kind, huge_string, stringlen, lineno);
		if (!tok)
			goto error;
		huge_string += stringlen;    // must be after new_token()
		if (dynamicarray_push(arr, (void *)tok))
			goto error;
	}
	return arr;

error:
	while (arr->len)
		token_free((struct Token *)dynamicarray_pop(arr));
	dynamicarray_free(arr);
	return NULL;
}
