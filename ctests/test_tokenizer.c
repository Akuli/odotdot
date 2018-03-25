#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <src/tokenizer.h>
#include <src/unicode.h>

#include "utils.h"


void test_tokenizer_read_file_to_huge_string(void)
{
	char s[] = "hellö\n\t";
	size_t n = strlen(s) + 1;    // INCLUDE the \0 this time
	s[2] = 0;   // even this must work

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


static char *unicode_to_utf8_ending_with_0(struct UnicodeString unicode)
{
	char *utf8;
	size_t utf8len;
	char errormsg[100];
	buttert(utf8_encode(unicode, &utf8, &utf8len, errormsg) == 0);
	buttert((utf8=realloc(utf8, utf8len+1)));
	utf8[utf8len]=0;
	return utf8;
}

struct Token *check_token(struct Token *tok, char kind, char *str, size_t lineno) {
	buttert(tok->kind == kind);
	buttert(tok->lineno == lineno);

	char *tokstr = unicode_to_utf8_ending_with_0(tok->str);
	buttert(strcmp(tokstr, str) == 0);
	free(tokstr);

	return tok->next;
}

void test_tokenizer_tokenize(void)
{
	char utf8code[] =
		"# cömment\n"
		"var abc = åäö;     \t #cömment \n"
		"print \"hellö wörld\"\n"
		";print 123 -123;\n"
		"2bäd;\n";
	int utf8codelen = strlen(utf8code);
	utf8code[5] = 0;    // must not break anything

	struct UnicodeString code;
	char errormsg[100] = {0};
	buttert(utf8_decode(utf8code, utf8codelen, &code, errormsg) == 0);
	buttert(errormsg[0] == 0);

	struct Token *tok1st = token_ize(code);
	buttert(tok1st);
	free(code.val);

	struct Token *curtok = tok1st;
	curtok = check_token(curtok, TOKEN_KEYWORD, "var", 2);
	curtok = check_token(curtok, TOKEN_ID, "abc", 2);
	curtok = check_token(curtok, TOKEN_OP, "=", 2);
	curtok = check_token(curtok, TOKEN_ID, "åäö", 2);
	curtok = check_token(curtok, TOKEN_OP, ";", 2);
	curtok = check_token(curtok, TOKEN_ID, "print", 3);
	curtok = check_token(curtok, TOKEN_STR, "\"hellö wörld\"", 3);
	curtok = check_token(curtok, TOKEN_OP, ";", 4);
	curtok = check_token(curtok, TOKEN_ID, "print", 4);
	curtok = check_token(curtok, TOKEN_INT, "123", 4);
	curtok = check_token(curtok, TOKEN_INT, "-123", 4);
	curtok = check_token(curtok, TOKEN_OP, ";", 4);
	curtok = check_token(curtok, TOKEN_INT, "2", 5);
	curtok = check_token(curtok, TOKEN_ID, "bäd", 5);
	curtok = check_token(curtok, TOKEN_OP, ";", 5);
	buttert(!curtok);

	token_freeall(tok1st);
}
