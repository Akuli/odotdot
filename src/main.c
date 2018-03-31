#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "tokenizer.h"
#include "unicode.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		assert(argc >= 1);
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		return 2;
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "%s: cannot open '%s'\n", argv[0], argv[1]);
		return 1;
	}

	char *hugestr;
	size_t hugestrlen;
	int status = read_file_to_huge_string(f, &hugestr, &hugestrlen);
	fclose(f);

	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while reading '%s'\n", argv[0], argv[1]);
		return 1;
	}
	if (status == 1) {
		fprintf(stderr, "%s: reading '%s' failed\n", argv[0], argv[1]);
		return 1;
	}
	assert(status == STATUS_OK);

	// convert to UnicodeString
	struct UnicodeString hugeunicode;
	char errormsg[100];
	status = utf8_decode(hugestr, hugestrlen, &hugeunicode, errormsg);
	free(hugestr);

	if (status == 1) {
		fprintf(stderr, "%s: error while decoding '%s' as UTF-8: %s\n", argv[0], argv[1], errormsg);
		return 1;
	}
	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while UTF8-decoding '%s'\n", argv[0], argv[1]);
		return 1;
	}
	assert(status == STATUS_OK);

	// tokenize
	struct Token *tok1st = token_ize(hugeunicode);
	free(hugeunicode.val);

	// TODO: parse and run instead of dumping the tokens to stdout
	for (struct Token *tok = tok1st; tok; tok = tok->next) {
		char errormsg[100]={0};
		char *tokval;
		size_t tokvallen;
		assert(utf8_encode(tok->str, &tokval, &tokvallen, errormsg) == STATUS_OK);
		assert((tokval = realloc(tokval, tokvallen+1)));
		tokval[tokvallen]=0;
		assert(!errormsg[0]);
		printf("%llu\t%c\t%s\n", (unsigned long long) tok->lineno, tok->kind, tokval);
		free(tokval);
	}

	token_freeall(tok1st);
	return 0;
}
