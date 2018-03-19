#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdio.h>   // for FILE

#define TOKENKIND_KEYWORD 'k'
#define TOKENKIND_ID 'x'
#define TOKENKIND_OP ';'
#define TOKENKIND_STRING '"'
#define TOKENKIND_INTEGER '1'

struct Token {
	char kind;
	unsigned long *val;
	size_t vallen;
	size_t lineno;
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
struct DynamicArray *token_ize(unsigned long *hugestring, size_t hugestringlen);
void token_free(struct Token *tok);

#endif   // TOKENIZER_H
