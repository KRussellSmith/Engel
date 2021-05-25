#ifndef memory_header
#define memory_header
#include <stddef.h>
#include "value.hpp"
#include "vm.hpp"
#define STRESS_GC
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))
#define FREE(type, val) \
	reallocate(val, sizeof(type), 0)
#define GROW(x) \
	((x) < 8 ? 8 : (x) * 2)
#define GROW_ARRAY(type, ptr, old, next) \
	(type*)reallocate(ptr, sizeof(type) * old, sizeof(type) * next)
#define FREE_ARRAY(type, ptr, old) \
	(type*)reallocate(ptr, sizeof(type) * old, 0);
void* reallocate(void* ptr, size_t old, size_t next);
void  mark_obj(Obj* obj);
void  mark_val(Value val);
void  collect(VM* vm);
void  free_objects(VM* vm);
#endif
