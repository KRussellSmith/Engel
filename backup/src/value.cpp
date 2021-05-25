#include "value.hpp"
#include "memory.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
void init_ValueArray(ValueArray* array)
{
	array->len = 0;
	array->cap = 0;
	array->values = NULL;
}
void push_ValueArray(ValueArray* array, Value value)
{
	if (array->len >= array->cap)
	{
		int old = array->cap;
		array->cap = GROW(old);
		array->values = GROW_ARRAY(Value, array->values, old, array->cap); 
	}
	array->values[array->len] = value;
	++array->len;
}
void free_ValueArray(ValueArray* array)
{
	FREE_ARRAY(Value, array->values, array->cap);
	init_ValueArray(array);
}
