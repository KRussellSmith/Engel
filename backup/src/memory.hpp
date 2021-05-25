#ifndef memory_header
#define memory_header
#include <stddef.h>
#define GROW(x) \
	((x) < 8 ? 8 : (x) * 2)
#define GROW_ARRAY(type, ptr, old, next) \
	(type*)reallocate(ptr, sizeof(type) * old, sizeof(type) * next)
#define FREE_ARRAY(type, ptr, old) \
	(type*)reallocate(ptr, sizeof(type) * old, 0);
void* reallocate(void* ptr, size_t old, size_t next);
#endif
