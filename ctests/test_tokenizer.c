#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <src/tokenizer.h>
#include <src/utf8.h>

#include "utils.h"


static char *unicode_to_utf8_ending_with_0(struct UnicodeString unicode)
{
	char *utf8;
	size_t utf8len;
	buttert(utf8_encode(testinterp, unicode, &utf8, &utf8len));
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
		";print 123 456;\n"
		"2bäd;\n";
	int utf8codelen = strlen(utf8code);
	utf8code[5] = 0;    // must not break anything

	struct UnicodeString code;
	buttert(utf8_decode(testinterp, utf8code, utf8codelen, &code) == true);

	struct Token *tok1st = token_ize(testinterp, code);
	buttert(tok1st);
	buttert(!testinterp->err);
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
	curtok = check_token(curtok, TOKEN_INT, "456", 4);
	curtok = check_token(curtok, TOKEN_OP, ";", 4);
	curtok = check_token(curtok, TOKEN_INT, "2", 5);
	curtok = check_token(curtok, TOKEN_ID, "bäd", 5);
	curtok = check_token(curtok, TOKEN_OP, ";", 5);
	buttert(!curtok);

	token_freeall(tok1st);
}
