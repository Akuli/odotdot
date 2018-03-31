#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "dynamicarray.h"
#include "tokenizer.h"
#include "unicode.h"

// functions can't be casted because c standards
static void astnode_free_voidstar(void *node)
{
	astnode_free(node);    // casts the node to struct AstNode*
}


// prints errors to stderr
// returns an exit code, e.g. 0 for success
static int run_file(char *argv0, char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "%s: cannot open '%s'\n", argv0, path);
		return 1;
	}

	char *hugestr;
	size_t hugestrlen;
	int status = read_file_to_huge_string(f, &hugestr, &hugestrlen);
	fclose(f);

	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while reading '%s'\n", argv0, path);
		return 1;
	}
	if (status == 1) {
		fprintf(stderr, "%s: reading '%s' failed\n", argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// convert to UnicodeString
	struct UnicodeString hugeunicode;
	char errormsg[100];
	status = utf8_decode(hugestr, hugestrlen, &hugeunicode, errormsg);
	free(hugestr);

	if (status == 1) {
		fprintf(stderr, "%s: the content of '%s' is not valid UTF-8: %s\n", argv0, path, errormsg);
		return 1;
	}
	if (status == STATUS_NOMEM) {
		fprintf(stderr, "%s: ran out of memory while UTF8-decoding the content of '%s'\n", argv0, path);
		return 1;
	}
	assert(status == STATUS_OK);

	// tokenize
	struct Token *tok1st = token_ize(hugeunicode);
	free(hugeunicode.val);

	// parse
	struct DynamicArray *statements = dynamicarray_new();
	if (!statements) {
		fprintf(stderr, "%s: ran out of memory while parsing the content of '%s'\n", argv0, path);
		token_freeall(tok1st);
		return 1;
	}

	struct Token *curtok = tok1st;
	while (curtok) {
		struct AstNode *st = parse_statement(&curtok);
		if (!st) {
			// TODO: better error reporting... also fix ast.c
			fprintf(stderr, "%s: something went wrong with parsing the content of '%s'\n", argv0, path);
			token_freeall(tok1st);
			return 1;
		}
		dynamicarray_push(statements, st);
	}
	token_freeall(tok1st);

	// TODO: run it instead of this
	if (statements->len == 1)
		printf("there is 1 statement\n");
	else
		printf("there are %llu statements\n", (unsigned long long) statements->len);
	dynamicarray_freeall(statements, astnode_free_voidstar);

	return 0;
}


int main(int argc, char **argv)
{
	if (argc != 2) {
		assert(argc >= 1);   // not sure if standards allow 0 args
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		return 2;
	}

	return run_file(argv[0], argv[1]);
}
