// TODO: use wchars?

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


static struct Token *new_token(struct Token *prev, char kind, unicode_t *val, size_t vallen, size_t lineno)
{
	struct Token *tok = malloc(sizeof (struct Token));
	if (!tok)
		return NULL;
	tok->val = malloc(vallen * sizeof(unicode_t));
	if (!(tok->val)) {
		free(tok);
		return NULL;
	}

	tok->kind = kind;
	memcpy(tok->val, val, vallen*sizeof(unicode_t));
	tok->vallen = vallen;
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
		free(tok->val);
		free(tok);
		tok = tok2;
	}
}


// TODO: better error handling than fprintf
// TODO: test the error cases :(
struct Token *token_ize(unicode_t *hugestring, size_t hugestringlen)
{
	size_t lineno=1;
	struct Token *tok1st = NULL;
	struct Token *curtok = NULL;
	char kind;
	size_t nchars;    // comparing size_t with size_t produces no warnings

	while (hugestringlen) {
		// TODO: check for any unicode whitespace
#define f(x) (hugestring[0]==(unicode_t)(x))
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
			kind = TOKEN_OP;
			nchars = 1;
		}

		else if (hugestringlen >= 4 &&
				hugestring[0] == (unicode_t)'v' &&
				hugestring[1] == (unicode_t)'a' &&
				hugestring[2] == (unicode_t)'r' &&
				!unicode_isalnum(hugestring[3])) {
			kind = TOKEN_KEYWORD;
			nchars = 3;
		}

		else if (hugestring[0] == '"') {
			kind = TOKEN_STR;
			nchars = 1;    // first "
			while ((hugestringlen > nchars) &&
					(hugestring[nchars] != (unicode_t)'"')) {
				if (hugestring[nchars] == (unicode_t)'\n') {
					fprintf(stderr, "line %llu: ending \" must be on the same line as starting \"", (unsigned long long)lineno);
					goto error;
				}
				nchars++;
			}
			nchars++;    // last ", the error handling stuff below runs if this is missing
		}

		else if (unicode_is0to9(hugestring[0])) {
			kind = TOKEN_INT;
			nchars = 0;
			while (hugestringlen > nchars && unicode_is0to9(hugestring[nchars]))
				nchars++;
		}

		else if (hugestringlen >= 2 && hugestring[0] == '-' && unicode_is0to9(hugestring[1])) {
			kind = TOKEN_INT;
			nchars = 1;
			while (hugestringlen > nchars && unicode_is0to9(hugestring[nchars]))
				nchars++;
		}

		else if (unicode_isalpha(hugestring[0])) {
			kind = TOKEN_ID;
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

		if (tok1st)
			curtok = new_token(curtok, kind, hugestring, nchars, lineno);
		else
			tok1st = curtok = new_token(NULL, kind, hugestring, nchars, lineno);
		if (!curtok)
			goto error;
		hugestring += nchars;    // must be after new_token()
		hugestringlen -= nchars;
	}

	// new_token sets nexts correctly for everything except the last token
	curtok->next = NULL;

	return tok1st;

error:
	if (tok1st)
		token_freeall(tok1st);
	return NULL;
}
