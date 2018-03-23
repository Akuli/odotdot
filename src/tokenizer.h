#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdio.h>
#include "unicode.h"

#define TOKEN_KEYWORD 'k'
#define TOKEN_ID 'x'
#define TOKEN_OP ';'
#define TOKEN_STR '"'
#define TOKEN_INT '1'

struct Token {
	char kind;
	struct UnicodeString str;
	size_t lineno;
	struct Token *next;
};

/* Read a file as chars.

Returns STATUS_OK, STATUS_NOMEM or -1 if reading the file failed.

On success, the file is read to *dest and its length as chars is set to
*destlen. If this fails, *dest and *destlen are not set.
*/
int read_file_to_huge_string(FILE *f, char **dest, size_t *destlen);

struct Token *token_ize(struct UnicodeString hugestring);
void token_freeall(struct Token *tok1st);

#endif   // TOKENIZER_H
