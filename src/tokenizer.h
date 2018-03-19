#ifndef TOKENIZER_H
#define TOKENIZER_H

#define TOKENKIND_KEYWORD 'k'
#define TOKENKIND_ID 'x'
#define TOKENKIND_OP ';'
#define TOKENKIND_STRING '"'
#define TOKENKIND_INTEGER '1'

struct Token {
	char kind;
	char *val;
	char val_firstchar;     // just for convenience
	size_t lineno;
};

char *read_file_to_huge_string(char *path);
struct DynamicArray *token_ize(char *huge_string);
void token_free(struct Token *tok);

#endif   // TOKENIZER_H
