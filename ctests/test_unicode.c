#include <src/common.h>
#include <src/unicode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void check(struct UnicodeString *s)
{
	buttert(s->len == 5);
	buttert(s->val[0] == 'h');
	buttert(s->val[1] == 'e');
	buttert(s->val[2] == 'l');
	buttert(s->val[3] == 'l');
	buttert(s->val[4] == 'o');
}

void test_copying(void)
{
	uint32_t avalues[] = { 'h', 'e', 'l', 'l', 'o' };
	struct UnicodeString a = { avalues, 5 };
	struct UnicodeString *b = unicodestring_copy(a);
	check(b);
	free(b->val);
	free(b);

	struct UnicodeString *c = bmalloc(sizeof(struct UnicodeString));
	buttert(unicodestring_copyinto(a, c) == STATUS_OK);
	check(c);
	free(c->val);
	free(c);
}


// this doesn't test the pink and red cells of the table in the wikipedia
// article because i don't understand the table :(
// https://en.wikipedia.org/wiki/UTF-8#Codepage_layout
struct Utf8Test {
	char utf8[100];
	int utf8len;
	uint32_t unicodeval[100];
	int unicodelen;    // -1 for no encode testing
	char errormsg[100];
};

#define N_UTF8_TESTS 13
struct Utf8Test utf8_tests[N_UTF8_TESTS];


void unicode_test_setup(void)
{
	// this weird macro system is just for standards
	// i used to have struct initializations like
	// {{ 0xf4, 0x8f, 0xbf, 0xbf }, 4, { 0x10FFFF }, 1, "" }
	// but -Wpedantic didn't like them
	int i=0;
#define INIT_ARRAY(type, dst, lendst, ...) do { \
		type tmp[] = { __VA_ARGS__ }; \
		lendst = sizeof(tmp)/sizeof(tmp[0]); \
		for (int j=0; j < lendst; j++) \
			dst[j] = tmp[j];\
	} while(0)
#define SET_UTF8(...) INIT_ARRAY(char, utf8_tests[i].utf8, utf8_tests[i].utf8len, __VA_ARGS__)
#define SET_UNICODE(...) INIT_ARRAY(uint32_t, utf8_tests[i].unicodeval, utf8_tests[i].unicodelen, __VA_ARGS__)
#define SET_NO_UNICODE() (utf8_tests[i].unicodelen = -1)
#define SET_ERROR(msg) strcpy(utf8_tests[i++].errormsg, (msg))
#define SET_NO_ERROR() SET_ERROR("")

	// very basic stuff
	SET_UTF8('h', 'e', 'l', 'l', 'o');
	SET_UNICODE('h', 'e', 'l', 'l', 'o');
	SET_NO_ERROR();

	// make sure that \0 is passed through as is
	SET_UTF8('h', 'e', 'l', 'l', 0, 'o');
	SET_UNICODE('h', 'e', 'l', 'l', 0, 'o');
	SET_NO_ERROR();

	// test empty string
	// can't do UTF8() or UNICODE() without arguments because they would produce
	// empty { } initializer, which is a KITTENS DIE thing according to the standards
	utf8_tests[i].utf8len = utf8_tests[i].unicodelen = 0;
	SET_NO_ERROR();

	// 1, 2, 3 and 4 byte utf8 characters
	// these char casts should work even if char is signed, then the values will
	// be negative when needed
	SET_UTF8(
		0x64,
		(char)0xcf,(char)0xa8,
		(char)0xe0,(char)0xae,(char)0xb8,
		(char)0xf0,(char)0x90,(char)0x85,(char)0x83);
	SET_UNICODE(100, 1000, 3000, 65859);
	SET_NO_ERROR();

	// euro sign with an overlong encoding, from wikipedia
	SET_UTF8((char)0xf0, (char)0x82, (char)0x82, (char)0xac);
	SET_NO_UNICODE();
	SET_ERROR("overlong encoding: 0xf0 0x82 0x82 0xac");

	// unexpected continuation byte, this is the euro sign without first byte
	SET_UTF8((char)0x82, (char)0x82, (char)0xac);
	SET_NO_UNICODE();
	SET_ERROR("invalid start byte 0x82");

	// again, same euro example, this time without last byte
	SET_UTF8((char)0xf0, (char)0x82, (char)0x82);
	SET_NO_UNICODE();
	SET_ERROR("unexpected end of string");

	// code points from U+D800 to U+DFFF are invalid
	SET_UTF8((char)0xed, (char)0x9f, (char)0xbf);
	SET_UNICODE(0xD800-1);
	SET_NO_ERROR();

	SET_UTF8((char)0xed, (char)0xa0, (char)0x80);
	SET_UNICODE(0xD800);
	SET_ERROR("invalid Unicode code point U+D800");

	SET_UTF8((char)0xed, (char)0xbf, (char)0xbf);
	SET_UNICODE(0xDFFF);
	SET_ERROR("invalid Unicode code point U+DFFF");

	SET_UTF8((char)0xee, (char)0x80, (char)0x80);
	SET_UNICODE(0xDFFF+1);
	SET_NO_ERROR();

	// code points bigger than U+10FFFF are invalid
	SET_UTF8((char)0xf4, (char)0x8f, (char)0xbf, (char)0xbf);
	SET_UNICODE(0x10FFFF);
	SET_NO_ERROR();

	SET_UTF8((char)0xf4, (char)0x90, (char)0x80, (char)0x80);
	SET_UNICODE(0x10FFFF+1);
	SET_ERROR("invalid Unicode code point U+110000");

	buttert(i == N_UTF8_TESTS);
#undef INIT_ARRAY
#undef SET_UTF8
#undef SET_UNICODE
#undef SET_NO_UNICODE
#undef SET_ERROR
#undef SET_NO_ERROR
}


void test_utf8_encode(void)
{
	for (size_t i=0; i < N_UTF8_TESTS; i++) {
		struct Utf8Test test = utf8_tests[i];
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

void test_utf8_decode(void)
{
	for (size_t i=0; i < N_UTF8_TESTS; i++) {
		struct Utf8Test test = utf8_tests[i];

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
