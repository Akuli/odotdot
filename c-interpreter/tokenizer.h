#ifndef TOKENIZER_H
#define TOKENIZER_H

#define TOKENKIND_KEYWORD 'k'
#define TOKENKIND_ID 'x'
#define TOKENKIND_OP ';'
#define TOKENKIND_STRING '"'
#define TOKENKIND_INTEGER '1'

struct Token {
	char kind;
	unsigned char *val;
	unsigned char val_firstchar;     // just for convenience
	size_t lineno;
};

unsigned char *read_file_to_huge_string(unsigned char *path);
struct DynamicArray *token_ize(unsigned char *huge_string);
void token_free(struct Token *tok);

#endif   // TOKENIZER_H
