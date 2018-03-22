#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdio.h>
#include "utf8.h"

#define TOKEN_KEYWORD 'k'
#define TOKEN_ID 'x'
#define TOKEN_OP ';'
#define TOKEN_STR '"'
#define TOKEN_INT '1'

struct Token {
	char kind;
	unicode_t *val;
	size_t vallen;
	size_t lineno;
	struct Token *next;
};

/* Read a file as chars.

Return values:

- `0`: success
- `-1`: not enough memory
- `1`: reading the file failed

On success, the file is read to `*dest` and its length as chars is set to
`*destlen`. If this fails, `*dest` and `*destlen` are not set.
*/
int read_file_to_huge_string(FILE *f, char **dest, size_t *destlen);
struct Token *token_ize(unicode_t *hugestring, size_t hugestringlen);
void token_freeall(struct Token *tok1st);

#endif   // TOKENIZER_H
