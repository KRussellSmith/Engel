#include "native.hpp"
#include "vm.hpp"
#include "value.hpp"
#include <stdio.h>
#define NATIVE(lib, name) Value lib::name(VM* vm, int num_args, Value* stack)
NATIVE(IO, print)
{
	for (int i = 0; i < num_args; ++i)
	{
		printf("%s\n", AS_CSTRING(vm->to_string_val(stack[i])));
	}
	return NULL_VAL;
}
#undef NATIVE
