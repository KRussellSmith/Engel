#include "value.hpp"
#include "memory.hpp"
#include "vm.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
void print_val(Value val)
{
	switch (val.type)
	{
		case VALUE_INT:
			printf("%ld", AS_INT(val));
			break;
		case VALUE_REAL:
			printf("%g", AS_REAL(val));
			break;
		case VALUE_OBJ:
			print_obj(AS_OBJ(val));
			break;
		case VALUE_BOOL:
			printf("%s", AS_BOOL(val)? "true" : "false");
			break;
		case VALUE_NULL:
			printf("null");
			break;
	}
}
void print_obj(Value val)
{
	switch (OBJ_TYPE(val))
	{
		case OBJ_STRING:
			printf("‘%s’", AS_CSTRING(val));
			break;
		case OBJ_FUNCTION:
			printf("<function>");
			break;
		case OBJ_NATIVE:
			printf("<native>");
			break;
		case OBJ_CLOSURE:
			printf("<closure>");
			break;
	}
}
static Obj* allocate_object(VM* vm, size_t size, ObjType type)
{
  auto object = (Obj*)reallocate(NULL, 0, size);

  object->type   = type;
  object->marked = false;
  object->next   = vm->objects;

  vm->objects = object;
	
  printf("%p allocate %zu for %d\n", (void*)object, size, type);

  return object;
}

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)allocate_object(vm, sizeof(type), objectType)
static uint32_t hash_string(const char* chars, int len)
{
	uint32_t hash = 2166136261u;

	for (int i = 0; i < len; ++i)
	{
		hash ^= chars[i];
		hash *= 16777619;
	}
	return hash;
}
Native* new_native(VM* vm, NativeFunc func)
{
	auto native = ALLOCATE_OBJ(vm, Native, OBJ_NATIVE);
	native->func = func;
	return native;
}

Function* new_function(VM* vm)
{
	auto function = ALLOCATE_OBJ(vm, Function, OBJ_FUNCTION);
	function->arity        = 0;
	function->num_upvalues = 0;
	function->name = NULL;
	init_Chunk(&function->chunk);
	return function;
}

Upvalue* new_upvalue(struct VM* vm, Value* slot)
{
	auto upvalue = ALLOCATE_OBJ(vm, Upvalue, OBJ_UPVALUE);

	upvalue->loc    = slot;
	upvalue->closed = NULL_VAL;
	upvalue->next   = NULL;
	return upvalue;
}

Closure* new_closure(VM* vm, Function* func)
{
	auto upvalues = ALLOCATE(Upvalue*, func->num_upvalues);
	for (int i = 0; i < func->num_upvalues; ++i)
	{
		upvalues[i] = NULL;
	}
	auto closure = ALLOCATE_OBJ(vm, Closure, OBJ_CLOSURE);
	closure->func = func;
	closure->upvalues = upvalues;
	closure->num_upvalues = func->num_upvalues;
	return closure;
}

static ObjString* new_string(
		VM* vm, char* chars, int length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
	string->len   = length;
	string->chars = chars;
	string->hash  = hash;
	put_map(&vm->strings, string, NULL_VAL);
	return string;
}
ObjString* copy_string(VM* vm, const char* chars, int length)
{
	auto hash = hash_string(chars, length);
	auto interned = map_find_str(&vm->strings, chars, length, hash);
	if (interned != NULL)
	{
		return interned;
	}
	auto heap = ALLOCATE(char, length + 1);
	memcpy(heap, chars, length);
	heap[length] = '\0';
	return new_string(vm, heap, length, hash);
}
ObjString* take_string(VM* vm, char* chars, int len)
{
	auto hash = hash_string(chars, len);
	auto interned = map_find_str(&vm->strings, chars, len, hash);
	if (interned != NULL)
	{
		return interned;
	}
	return new_string(vm, chars, len, hash);
}

Map* new_map(VM* vm)
{
	Map* map = ALLOCATE_OBJ(vm, Map, OBJ_MAP);
	init_map(map);
	return map;
}
void init_map(Map* map)
{
	map->len     = 0;
	map->cap     = 0;
	map->entries = NULL;
}
static MapEntry* find_map_entry(MapEntry* entries, int cap, ObjString* key)
{
	uint32_t index = key->hash % cap;
	MapEntry* tombstone = NULL;
	for (;;)
	{
		auto entry = &entries[index];
		if (entry->key == NULL)
		{
			if (IS_NULL(entry->value))
			{
				return tombstone != NULL ? tombstone : entry;
			}
			else
			{
				if (tombstone == NULL)
				{
					tombstone = entry;
				}
			}
		}
		else if (entry->key == key)
		{
			return entry;
		}

		index = (index + 1) % cap;
	}
}
static void adjust_cap(Map* map, int cap)
{
	auto entries = ALLOCATE(MapEntry, cap);
	for (int i = 0; i < cap; ++i)
	{
		entries[i].key = NULL,
		entries[i].value = NULL_VAL;
	}
	map->len = 0;
	for (int i = 0; i < map->cap; ++i)
	{
		auto entry = &map->entries[i];
		if (entry->key == NULL)
		{
			continue;
		}
		auto dest = find_map_entry(entries, cap, entry->key);
		dest->key   = entry->key;
		dest->value = entry->value;
		++map->len;
	}
	
	FREE_ARRAY(MapEntry, map->entries, map->cap);
	map->entries = entries;
	map->cap     = cap;
}
bool put_map(Map* map, ObjString* key, Value value)
{
	if (map->len + 1 > map->cap * 0.75)
	{
		int cap = GROW(map->cap);
		adjust_cap(map, cap);
	}
	auto entry = find_map_entry(map->entries, map->cap, key);
	auto is_new = entry->key == NULL;
	if (is_new && IS_NULL(entry->value))
	{
		++map->len;
	}
	entry->key   = key;
	entry->value = value;
	return is_new;
}
void copy_map(Map* from, Map* to)
{
	for (int i = 0; i < from->cap; ++i)
	{
		auto entry = &from->entries[i];
		if (entry->key != NULL)
		{
			put_map(to, entry->key, entry->value);
		}
	}
}
bool get_map(Map* map, ObjString* key, Value* value)
{
	if (map->len == 0)
	{
		return false;
	}
	auto entry = find_map_entry(map->entries, map->cap, key);
	if (entry->key == NULL)
	{
		return false;
	}
	*value = entry->value;
	return true;
}
bool rm_map(Map* map, ObjString* key)
{
	if (map->len == 0)
	{
		return false;
	}
	auto entry = find_map_entry(map->entries, map->cap, key);
	if (entry->key == NULL)
	{
		return false;
	}

	entry->key = NULL;
	entry->value = BOOL_VAL(true);

	return true;
}
ObjString* map_find_str(
		Map* map, const char* chars, int len, uint32_t hash)
{
	if (map->len == 0)
	{
		return NULL;
	}
	uint32_t index = hash % map->cap;
	for (;;)
	{
		auto entry = &map->entries[index];
		if (entry->key == NULL)
		{
			if (IS_NULL(entry->value))
			{
				return NULL;
			}
		}
		else if (
				entry->key->len  == len &&
				entry->key->hash == hash &&
				memcmp(entry->key->chars, chars, len) == 0)
		{
			return entry->key;
		}
		index = (index + 1) % map->cap;
	}
}
void free_map(Map* map)
{
	FREE_ARRAY(MapEntry, map->entries, map->cap);
	init_map(map);
}

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
