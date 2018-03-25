#include "utils.h"

void *bmalloc(size_t size)
{
	void *res = malloc(size);
	buttert2(res, "not enough mem :(");
	return res;
}
