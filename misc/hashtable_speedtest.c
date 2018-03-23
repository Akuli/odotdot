/* compile and run like this (in project root):

      $ make misc-compiled/hashtable_speedtest && misc-compiled/hashtable_speedtest
*/

#include <assert.h>
#include <limits.h>
#include <sys/time.h>   // posix only, and gettimeofday is "obsolete" according to my man page :(
#include <stdio.h>
#include <stdlib.h>
#include <src/common.h>
#include <src/hashtable.h>

#define NITEMS 1000
#define NFILLCLEARTIMES 100
#define NTOTALTIMES 100

#define USEC "\xc2\xb5s"


unsigned long inthash(int i) {
	return i + 20;
}
int intcmp(void *a, void *b, void *userdata)
{
	return (*((int *)a)) == (*((int *)b));
}

// lol
#define repeat(howmanytimes) int _counter = howmanytimes; while (_counter--)

void stats(unsigned long *data, int datalen)
{
	unsigned long min = ULONG_MAX;
	unsigned long max = 0;
	unsigned long avg = 0;
	for (int i=0;i<datalen;i++) {
		if (data[i] < min)
			min = data[i];
		if (data[i] > max)
			max = data[i];
		avg+=data[i];
	}
	avg /= datalen;

	printf("%d runs, min %lu"USEC", max %lu"USEC", average %lu"USEC"\n", datalen, min, max, avg);
}

int main(void)
{
	int *keys[NITEMS];
	int keyhashes[NITEMS];
	char *values[NITEMS];
	unsigned long data4stats[NTOTALTIMES];

	struct HashTable *ht = hashtable_new(intcmp);
	assert(ht);
	for (int i = 0; i < NITEMS; i++) {
		keys[i] = malloc(sizeof(int));
		values[i] = malloc(10);
		assert(keys[i] && values[i]);
		*keys[i] = i;
		keyhashes[i] = inthash(i);
		sprintf(values[i], "lol %d", i);
	}

	for (int totali=0; totali < NTOTALTIMES; totali++) {
		struct timeval start, end;
		gettimeofday(&start, NULL);

		char *ptr;
		for (int filli=0; filli < NFILLCLEARTIMES; filli++) {
			for (int i = 0; i < NITEMS; i++) {
				assert(hashtable_set(ht, keys[i], keyhashes[i], values[i], NULL) == STATUS_OK);
			}
			assert(ht->size == NITEMS);
			for (int i = 0; i < NITEMS; i++) {
				assert(hashtable_get(ht, keys[i], keyhashes[i], (void **)(&ptr), NULL) == 1);
				assert(ptr == values[i]);
			}
			assert(ht->size == NITEMS);
			hashtable_clear(ht);
			assert(ht->size == 0);
		}
		gettimeofday(&end, NULL);
		data4stats[totali] = (end.tv_sec - start.tv_sec)*1000*1000 + (end.tv_usec - start.tv_usec);
	}
	stats(data4stats, NTOTALTIMES);

	hashtable_free(ht);
	for (int i = 0; i < NITEMS; i++) {
		free(keys[i]);
		free(values[i]);
	}
	return 0;
}
