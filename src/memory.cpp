#include "memory.hpp"
#include <stdlib.h>
#include "vm.hpp"
#include "value.hpp"
void* reallocate(void* ptr, size_t old, size_t next)
{
	if (next > old)
	{
		#ifdef STRESS_GC
		collect();
		#endif
	}
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
static void free_obj(Obj* object)
{
	printf("%p free type %d\n", (void*)object, object->type);
	switch (object->type)
	{
		case OBJ_FUNCTION:
		{
			auto func = (Function*)object;
			free_Chunk(&func->chunk);
			FREE(Function, object);
			break;
		}
		case OBJ_CLOSURE:
		{
			auto closure = (Closure*)object;
			FREE_ARRAY(Upvalue*, closure->upvalues, closure->num_upvalues);
			FREE(Closure, object);
			break;
		}
		case OBJ_UPVALUE:
		{
			FREE(Upvalue, object);
			break;
		}
		case OBJ_STRING:
		{
			ObjString* string = (ObjString*)object;
			FREE_ARRAY(char, string->chars, string->len + 1);
			FREE(ObjString, object);
			break;
		}
		case OBJ_NATIVE:
		{
			FREE(Native, object);
			break;
		}
	}
}
static void mark_roots(VM* vm)
{
	for (auto slot = vm->stack; slot < vm->top; ++slot)
	{
		mark_val(*slot);
	}
}
void  mark_obj(Obj* obj)
{
	if (obj == NULL)
	{
		return;
	}
	printf("%p mark ", (void*)obj);
	print_val(OBJ_VAL(obj));
	printf("\n");
	obj->marked = true;
}
void mark_val(Value val)
{
	if (!IS_OBJ(val))
	{
		return;
	}
	mark_obj(AS_OBJ(val));
}
void collect(VM* vm)
{
	puts("Begin GC");
	mark_roots(vm);
	puts("End GC");
}
void free_objects(VM* vm)
{
  Obj* object = vm->objects;
  while (object != NULL)
  {
    Obj* next = object->next;
    free_obj(object);
    object = next;
  }
}
