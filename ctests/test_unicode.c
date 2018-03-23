#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include <src/unicode.h>


struct UnicodeTest {
	char utf8[100];
	int utf8len;
	uint32_t unicodeval[100];
	int unicodelen;    // -1 for no encode testing
	char errormsg[100];
};

// this doesn't test the pink and red cells of the table in the wikipedia
// article because i don't understand the table :(
// https://en.wikipedia.org/wiki/UTF-8#Codepage_layout
struct UnicodeTest unicode_tests[] = {
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
	{{ 0xee, 0x80, 0x80 }, 3, { 0xDFFF+1 }, 1, "" },

	// code points bigger than U+10FFFF are invalid
	{{ 0xf4, 0x8f, 0xbf, 0xbf }, 4, { 0x10FFFF }, 1, "" },
	{{ 0xf4, 0x90, 0x80, 0x80 }, 4, { 0x10FFFF+1 }, 1, "invalid Unicode code point U+110000" }
};


void test_utf8_decode(void)
{
	for (size_t i=0; i < sizeof(unicode_tests)/sizeof(unicode_tests[0]); i++) {
		struct UnicodeTest test = unicode_tests[i];

		char errormsg[100] = {0};
		struct UnicodeString actual_unicode;
		actual_unicode.len = 123;
		actual_unicode.val = (uint32_t*)0xdeadbeef;   // lol
		int res = utf8_decode(test.utf8, (size_t) test.utf8len, &actual_unicode, errormsg);

		if (strlen(test.errormsg) == 0) {
			// should succeed
			buttert(res == 0);
			buttert(actual_unicode.len == (size_t) test.unicodelen);
			buttert(memcmp(test.unicodeval, actual_unicode.val, sizeof(uint32_t)*test.unicodelen) == 0);
			for (int i=0; i < 100; i++)
				buttert(errormsg[i] == 0);
			free(actual_unicode.val);
		} else {
			// should fail
			buttert(res == 1);
			buttert(strcmp(errormsg, test.errormsg) == 0);

			// these must be left untouched
			buttert(actual_unicode.val == (uint32_t *) 0xdeadbeef);
			buttert(actual_unicode.len == 123);
		}
	}
}


void test_utf8_encode(void)
{
	for (size_t i=0; i < sizeof(unicode_tests)/sizeof(unicode_tests[0]); i++) {
		struct UnicodeTest test = unicode_tests[i];
		if (test.unicodelen < 0)
			continue;

		char errormsg[100] = {0};
		char *actual_utf8 = (char *) 0xdeadbeef;
		size_t actual_utf8len = 123;
		struct UnicodeString unicode = { test.unicodeval, (size_t) test.unicodelen };
		int res = utf8_encode(unicode, &actual_utf8, &actual_utf8len, errormsg);

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

TESTS_MAIN(test_utf8_encode, test_utf8_decode, NULL);
