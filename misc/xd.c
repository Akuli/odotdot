// my unicode version of xxd
// written in C because i can't use ö to compile the ö interpreter
// and i don't want dependencies other than a c compiler and build tools
// error handling is assert because i don't feel like doing it better :D

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define CHUNKSIZE 4096


int main(void)
{
	char c;
	unsigned char *bytes = NULL;
	size_t byteslen = 0;
	size_t allocated = 0;
	while ((c = fgetc(stdin)) != EOF) {
		if (byteslen == allocated) {
			assert((bytes = realloc(bytes, allocated+CHUNKSIZE)));
			allocated += CHUNKSIZE;
		}
		bytes[byteslen++] = c;
	}
	assert(!ferror(stdin));

	// the only non-ASCIIs in builtins.ö are Ö and ö
	// int is at least 16 bits, which is plenty for Ö and ö
	// there are always less unicode characters than bytes
	unsigned int *unicode = malloc(sizeof(unsigned int) * byteslen);
	assert(unicode);
	size_t unicodelen = 0;   // always less than byteslen

	for (size_t i=0; i < byteslen; ) {
		if (bytes[i] < 128)   // ASCII
			unicode[unicodelen++] = bytes[i++];
		else if (bytes[i] == 0xc3) {   // beginning of Ö or ö
			if (bytes[i+1] == 0x96)   // Ö
				unicode[unicodelen++] = 0xd6;
			else if (bytes[i+1] == 0xb6)    // ö
				unicode[unicodelen++] = 0xf6;
			else
				assert(0);
			i += 2;
		} else
			assert(0);
	}
	free(bytes);

	for (size_t i=0; i < unicodelen; i++){
		printf("%#x", unicode[i]);
		if (i != unicodelen-1)
			printf(", ");
	}
	free(unicode);
	printf("\n");

	return 0;
}
