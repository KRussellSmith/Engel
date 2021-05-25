#include "memory.h"
#include <stdlib.h>
void* reallocate(void* ptr, size_t old, size_t next)
{
	if (next == 0)
	{
		free(ptr);
		return NULL;
	}
	void* result = realloc(ptr, next);
	if (result == NULL)
	{
		exit(1);
	}
	return result;
}
