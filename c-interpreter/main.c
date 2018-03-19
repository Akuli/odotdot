#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynamicarray.h"
#include "tokenizer.h"
#include "utf8.h"


// minimal test framework
#define BEGIN_TESTS void runtests(void) {
#define TEST(name) printf("testing %s...\n", #name);
#define END_TESTS } int main(void) { runtests(); printf("ok\n"); return 0; }

// these are not called assert because assert would conflict with assert.h
// instead, replace ASS in ASSert with BUTT
#define buttert2(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "assertion '%s' failed in %s:%d, function '%s': %s\n", \
			#cond, __FILE__, __LINE__, __func__, msg); \
		if (msg != NULL) { \
		} \
		abort(); \
	} \
} while (0)
#define buttert(cond) buttert2(cond, "")


/* this python script was used for generating some of these tests

import random

found = 0

while found < 30:
    data = bytes(random.randint(0, 0xff) for junk in range(10))
    try:
        data.decode('utf-8')
    except UnicodeDecodeError:
        print('{{ %s }, %d, "", -1 }' % (', '.join(map(hex, data)), len(data)))
        found += 1
*/

struct Utf8Test {
	char utf8[100];
	int utf8len;
	unsigned long unicode[100];
	int unicodelen;    // -1 for no encode testing
	char errormsg[100];
};

// this doesn't test the pink and red cells of the table in the wikipedia
// article because i don't understand the table :(
// https://en.wikipedia.org/wiki/UTF-8#Codepage_layout
struct Utf8Test utf8_tests[] = {
	// very basic stuff
	{{ 'h', 'e', 'l', 'l', 'o' }, 5, { 'h', 'e', 'l', 'l', 'o' }, 5, "" },
	{{ }, 0, { }, 0, "" },

	// 1, 2, 3 and 4 byte utf8 characters
	{{ 0x64, 0xcf,0xa8, 0xe0,0xae,0xb8, 0xf0,0x90,0x85,0x83 }, 1+2+3+4, { 100, 1000, 3000, 65859 }, 4, "" },

	// euro sign with an overlong encoding, from wikipedia
	{{ 0xf0, 0x82, 0x82, 0xac }, 4, { }, -1, "overlong encoding: 0xf0 0x82 0x82 0xac" },

	// unexpected continuation byte, this is the euro thing above without first byte
	{{ 0x82, 0x82, 0xac }, 3, { }, -1, "invalid start byte 0x82" },

	// again, same euro example, this time without last byte
	{{ 0xf0, 0x82, 0x82 }, 3, { }, -1, "unexpected end of string" },

	// code points from U+D800 to U+DFFF are invalid
	{{ 0xed, 0x9f, 0xbf }, 3, { 0xD800-1 }, 1, "" },
	{{ 0xed, 0xa0, 0x80 }, 3, { 0xD800 }, 1, "invalid Unicode code point U+D800" },
	{{ 0xed, 0xbf, 0xbf }, 3, { 0xDFFF }, 1, "invalid Unicode code point U+DFFF" },
	{{ 0xee, 0x80, 0x80 }, 3, { 0xDFFF+1 }, 1, "" }
};

static void token_free_voidstar(void *tok) { token_free((struct Token *)tok); }    // because c standards


BEGIN_TESTS;

TEST(utf8_decode) {
	for (size_t i=0; i < sizeof(utf8_tests)/sizeof(utf8_tests[0]); i++) {
		struct Utf8Test test = utf8_tests[i];

		char errormsg[100] = {0};
		unsigned long *actual_unicode = (unsigned long *) 0xdeadbeef;   // lol
		size_t actual_unicodelen = 123;
		int res = utf8_decode(test.utf8, (size_t) test.utf8len, &actual_unicode, &actual_unicodelen, errormsg);

		if (strlen(test.errormsg) == 0) {
			// should succeed
			buttert(res == 0);
			buttert(actual_unicodelen == (size_t) test.unicodelen);
			buttert(memcmp(test.unicode, actual_unicode, sizeof(long)*test.unicodelen) == 0);
			for (int i=0; i < 100; i++)
				buttert(errormsg[i] == 0);
			free(actual_unicode);
		} else {
			// should fail
			buttert(res == 1);
			buttert(strcmp(errormsg, test.errormsg) == 0);

			// these must be left untouched
			buttert(actual_unicode == (unsigned long *) 0xdeadbeef);
			buttert(actual_unicodelen == 123);
		}
	}
}


TEST(utf8_decode) {
	for (size_t i=0; i < sizeof(utf8_tests)/sizeof(utf8_tests[0]); i++) {
		struct Utf8Test test = utf8_tests[i];
		if (test.unicodelen < 0)
			continue;

		char errormsg[100] = {0};
		char *actual_utf8 = (char *) 0xdeadbeef;
		size_t actual_utf8len = 123;
		int res = utf8_encode(test.unicode, (size_t) test.unicodelen, &actual_utf8, &actual_utf8len, errormsg);

		if (strlen(test.errormsg) == 0) {
			// should succeed
			buttert(res == 0);
			buttert(actual_utf8len == (size_t) test.utf8len);
			buttert(memcmp(test.utf8, actual_utf8, test.utf8len) == 0);
			for (int i=0; i < 100; i++)
				buttert(errormsg[i] == 0);
			free(actual_utf8);
		} else {
			// should fail
			buttert(res == 1);
			buttert(strcmp(errormsg, test.errormsg) == 0);

			// these must be left untouched
			buttert(actual_utf8 == (char *) 0xdeadbeef);
			buttert(actual_utf8len == 123);
		}
	}
}


TEST(dynamicarray) {
	struct DynamicArray *arr = dynamicarray_new();
	buttert(arr->len == 0);

	for (int i=1; i <= 100; i++) {
		int *ip = malloc(sizeof(int));
		buttert(ip);
		*ip = i;
		dynamicarray_push(arr, ip);
	}
	buttert(arr->len == 100);

	for (int i=100; i > 50; i--) {
		int *ip = dynamicarray_pop(arr);
		buttert(*ip == i);
		free(ip);
	}
	buttert(arr->len == 50);

	dynamicarray_freeall(arr, free);
	// cannot check arr->len anymore, arr has been freed
}


TEST(read_file_to_huge_string_and_tokenize) {
	char *huge_string = read_file_to_huge_string("test.ö");
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

END_TESTS;
