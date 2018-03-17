#include <stdio.h>
#include <stdlib.h>
#include "dynamicarray.h"
#include "tokenizer.h"

// because c standards
static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }

int main(void)
{
	unsigned char *huge_string;
	if (!(huge_string = read_file_to_huge_string((unsigned char *)"test.ö"))) {
		fprintf(stderr, "reading test.ö failed\n");
		goto error;
	}

	struct DynamicArray *arr = token_ize(huge_string);
	if (!arr) {
		fprintf(stderr, "tokenizing failed\n");
		goto error;
	}

	free(huge_string);
	huge_string = NULL;

	for (size_t i=0; i < arr->len; i++) {
		struct Token *tok = (struct Token *)(arr->values[i]);
		printf("%lu\t%c\t%s\n", (unsigned long) tok->lineno, tok->kind, tok->val);
	}
	dynamicarray_free2(arr, token_free_voidstar);
	return 0;

error:
	if (huge_string)
		free(huge_string);
	return 1;
}
