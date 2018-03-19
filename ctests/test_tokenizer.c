#include <stddef.h>

#include <src/dynamicarray.h>
#include <src/tokenizer.h>

#include "utils.h"

// because c standards
static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }


BEGIN_TESTS

TEST(read_file_to_huge_string_and_tokenize) {
	char *huge_string = read_file_to_huge_string("ctests/test.ö");
	buttert(huge_string);

	struct DynamicArray *arr = token_ize(huge_string);
	buttert2(arr, "no mem :(");

	free(huge_string);

	for (size_t i=0; i < arr->len; i++) {
		struct Token *tok = (struct Token *)(arr->values[i]);
		(void)tok;
		//printf("%llu\t%c\t%s\n", (unsigned long long) tok->lineno, tok->kind, tok->val);
	}
	dynamicarray_freeall(arr, token_free_voidstar);
}

END_TESTS
