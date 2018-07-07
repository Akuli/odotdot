#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include "interpreter.h"    // IWYU pragma: keep
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

struct Token *token_ize(struct Interpreter *interp, struct UnicodeString hugestring);
void token_freeall(struct Token *tok1st);

#endif   // TOKENIZER_H
