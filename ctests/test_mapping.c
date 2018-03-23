#include <src/common.h>
#include <src/mapping.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int intcmp(void *a, void *b, void *userdata)
{
	buttert(userdata == (void *)0xdeadbeef);
	return (*((int *)a)) == (*((int *)b));
}

unsigned long inthash(int i) {
	return i + 20;
}

BEGIN_TESTS

TEST(basic_stuff) {
	int i=1, j=2, k=3;
	int *ptr;
	struct Mapping *map = mapping_new(intcmp);
	buttert(map);
	buttert(mapping_set(map, &i, inthash(i), &j, (void *)0xdeadbeef) == STATUS_OK);
	buttert(mapping_get(map, &i, inthash(i), (void **)(&ptr), (void *)0xdeadbeef) == 1);
	buttert(ptr == &j && j == 2);

	buttert(mapping_set(map, &i, inthash(i), &k, (void *)0xdeadbeef) == STATUS_OK);
	buttert(mapping_get(map, &i, inthash(i), (void **)(&ptr), (void *)0xdeadbeef) == 1);
	buttert(ptr == &k && k == 3);

	mapping_free(map);
}

#define MANY 1000
TEST(many_values) {
	int *keys[MANY];
	int keyhashes[MANY];
	char *values[MANY];

	struct Mapping *map = mapping_new(intcmp);
	buttert(map);
	for (int i = 0; i < MANY; i++) {
		keys[i] = bmalloc(sizeof(int));
		*keys[i] = i;
		keyhashes[i] = inthash(i);
		values[i] = bmalloc(10);
		sprintf(values[i], "lol %d", i);
	}

	char *ptr;
	int counter = 3;
	while (counter--) {   // repeat 3 times
		for (int i = 0; i < MANY; i++) {
			buttert(mapping_set(map, keys[i], keyhashes[i], values[i], (void *)0xdeadbeef) == STATUS_OK);
		}
		buttert(map->size == MANY);
		for (int i = 0; i < MANY; i++) {
			buttert(mapping_get(map, keys[i], keyhashes[i], (void **)(&ptr), (void *)0xdeadbeef) == 1);
			buttert(ptr == values[i]);
		}
		buttert(map->size == MANY);
		mapping_clear(map);
		buttert(map->size == 0);
	}

	mapping_free(map);
	for (int i = 0; i < MANY; i++) {
		free(keys[i]);
		free(values[i]);
	}
}
#undef MANY

END_TESTS
