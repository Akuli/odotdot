// TODO: fix tokenizing empty files
#include "tokenizer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "unicode.h"


#define CHUNK_SIZE 4096
int read_file_to_huge_string(FILE *f, char **dest, size_t *destlen)
{
	int errorcode=STATUS_OK;

	char *s = NULL;
	char buf[CHUNK_SIZE];
	size_t totalsize=0;
	size_t nread;
	while ((nread = fread(buf, 1, CHUNK_SIZE, f))) {
		// <jp> if s is NULL in realloc(s, sz), it acts like malloc
		char *ptr = realloc(s, totalsize+nread);
		if (!ptr) {
			errorcode = STATUS_NOMEM;
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
	return STATUS_OK;

error:
	if (s)
		free(s);
	return errorcode;
}
#undef CHUNK_SIZE


// creates a token of first nchars chars of hugestring
static struct Token *new_token(struct Token *prev, char kind, struct UnicodeString hugestring, size_t nchars, size_t lineno)
{
	struct Token *tok = malloc(sizeof (struct Token));
	if (!tok)
		return NULL;

	hugestring.len = nchars;    // it's pass-by-value
	if (unicodestring_copyinto(hugestring, &(tok->str)) != STATUS_OK) {
		free(tok);
		return NULL;
	}

	tok->kind = kind;
	tok->lineno = lineno;
	if (prev)
		prev->next = tok;
	return tok;
}


void token_freeall(struct Token *tok1st)
{
	struct Token *tok = tok1st;
	while (tok) {
		struct Token *tok2 = tok->next;   // must be before free(tok)
		free(tok->str.val);
		free(tok);
		tok = tok2;
	}
}


// TODO: better error handling than fprintf
// TODO: test the error cases :(
struct Token *token_ize(struct UnicodeString hugestring)
{
	size_t lineno=1;
	struct Token *tok1st = NULL;
	struct Token *curtok = NULL;
	char kind;
	size_t nchars;    // comparing size_t with size_t produces no warnings

	while (hugestring.len) {
		// TODO: check for any unicode whitespace
#define f(x) (hugestring.val[0]==(x))
		if (f(' ')||f('\t')||f('\n')) {
			if (f('\n'))
				lineno++;
			// this relies on pass-by-value
			hugestring.val++;
			hugestring.len--; 
			continue;
		}

		if (hugestring.val[0] == '#') {
			while (hugestring.len && hugestring.val[0] != '\n') {
				hugestring.val++;
				hugestring.len--;
			}
			continue;
		}

		if (f('{')||f('}')||f('[')||f(']')||f('(')||f(')')||f('=')||f(';')||f('.')) {
#undef f
			kind = TOKEN_OP;
			nchars = 1;
		}

		else if (hugestring.len >= 4 &&
				hugestring.val[0] == 'v' &&
				hugestring.val[1] == 'a' &&
				hugestring.val[2] == 'r' &&
				!unicode_isidentifiernot1st(hugestring.val[3])) {
			kind = TOKEN_KEYWORD;
			nchars = 3;
		}

		else if (hugestring.val[0] == '"') {
			kind = TOKEN_STR;
			nchars = 1;    // first "
			while ((hugestring.len > nchars) &&
					(hugestring.val[nchars] != '"')) {
				if (hugestring.val[nchars] == '\n') {
					fprintf(stderr, "line %llu: ending \" must be on the same line as starting \"", (unsigned long long)lineno);
					goto error;
				}
				nchars++;
			}
			nchars++;    // last ", the error handling stuff below runs if this is missing
		}

		else if (unicode_is0to9(hugestring.val[0])) {
			kind = TOKEN_INT;
			nchars = 0;
			while (hugestring.len > nchars && unicode_is0to9(hugestring.val[nchars]))
				nchars++;
		}

		else if (hugestring.len >= 2 && hugestring.val[0] == '-' && unicode_is0to9(hugestring.val[1])) {
			kind = TOKEN_INT;
			nchars = 1;
			while (hugestring.len > nchars && unicode_is0to9(hugestring.val[nchars]))
				nchars++;
		}

		else if (unicode_isidentifier1st(hugestring.val[0])) {
			kind = TOKEN_ID;
			nchars = 0;
			while (hugestring.len > nchars && unicode_isidentifiernot1st(hugestring.val[nchars]))
				nchars++;
		}

		else {
			fprintf(stderr, "line %llu: unknown token\n", (unsigned long long)lineno);
			goto error;
		}

		if (hugestring.len < nchars) {
			fprintf(stderr, "unexpected end of file\n");
			goto error;
		}

		if (tok1st)
			curtok = new_token(curtok, kind, hugestring, nchars, lineno);
		else
			tok1st = curtok = new_token(NULL, kind, hugestring, nchars, lineno);
		if (!curtok)
			goto error;
		hugestring.val += nchars;    // must be after new_token()
		hugestring.len -= nchars;
	}

	// new_token sets nexts correctly for everything except the last token
	curtok->next = NULL;

	return tok1st;

error:
	if (tok1st)
		token_freeall(tok1st);
	return NULL;
}
