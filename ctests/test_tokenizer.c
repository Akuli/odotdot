#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <src/dynamicarray.h>
#include <src/tokenizer.h>
#include <src/utf8.h>

#include "utils.h"

// because c standards
static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }

static char *unicode_to_utf8_ending_with_0(unsigned long *unicode, size_t unicodelen)
{
	char *utf8;
	size_t utf8len;
	char errormsg[100];
	buttert(utf8_encode(unicode, unicodelen, &utf8, &utf8len, errormsg) == 0);
	buttert((utf8=realloc(utf8, utf8len+1)));
	utf8[utf8len]=0;
	return utf8;
}


void check_token(struct Token *tok, char kind, char *val, size_t lineno) {
	buttert(tok->kind == kind);
	buttert(tok->lineno == lineno);

	char *tokval = unicode_to_utf8_ending_with_0(tok->val, tok->vallen);
	buttert(strcmp(tokval, val) == 0);
	free(tokval);
}


BEGIN_TESTS

TEST(read_file_to_huge_string) {
	char s[] = "hellö\n\t";
	size_t n = strlen(s) + 1;    // INCLUDE the \0 this time

	FILE *f = tmpfile();
	buttert(f);
	buttert(fwrite(s, 1, n, f));
	rewind(f);   // the standards don't guarantee that this works :(

	size_t n2;
	char *s2;
	read_file_to_huge_string(f, &s2, &n2);
	buttert(fclose(f) == 0);

	buttert(n == n2);
	buttert(memcmp(s, s2, n) == 0);

	free(s2);
}


TEST(tokenize) {
	char utf8code[] =
		"# cömment\n"
		"var abc = åäö;     \t #cömment \n"
		"print \"hellö wörld\"\n"
		";print 123 -123;\n"
		"2bäd;\n";
	int utf8codelen = strlen(utf8code);
	utf8code[5] = 0;    // must not break anything

	unsigned long *code;
	size_t codelen;
	char errormsg[100] = {0};
	buttert(utf8_decode(utf8code, utf8codelen, &code, &codelen, errormsg) == 0);
	buttert(errormsg[0] == 0);

	struct DynamicArray *tokens = token_ize(code, codelen);
	buttert(tokens);
	free(code);

	size_t i=0;
	check_token(tokens->values[i++], TOKENKIND_KEYWORD, "var", 2);
	check_token(tokens->values[i++], TOKENKIND_ID, "abc", 2);
	check_token(tokens->values[i++], TOKENKIND_OP, "=", 2);
	check_token(tokens->values[i++], TOKENKIND_ID, "åäö", 2);
	check_token(tokens->values[i++], TOKENKIND_OP, ";", 2);
	check_token(tokens->values[i++], TOKENKIND_ID, "print", 3);
	check_token(tokens->values[i++], TOKENKIND_STRING, "\"hellö wörld\"", 3);
	check_token(tokens->values[i++], TOKENKIND_OP, ";", 4);
	check_token(tokens->values[i++], TOKENKIND_ID, "print", 4);
	check_token(tokens->values[i++], TOKENKIND_INTEGER, "123", 4);
	check_token(tokens->values[i++], TOKENKIND_INTEGER, "-123", 4);
	check_token(tokens->values[i++], TOKENKIND_OP, ";", 4);
	check_token(tokens->values[i++], TOKENKIND_INTEGER, "2", 5);
	check_token(tokens->values[i++], TOKENKIND_ID, "bäd", 5);
	check_token(tokens->values[i++], TOKENKIND_OP, ";", 5);
	buttert(tokens->len == i);

	dynamicarray_freeall(tokens, token_free_voidstar);
}

END_TESTS
