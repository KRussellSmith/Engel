#ifndef native_header
#define native_header
#include "vm.hpp"
#include "value.hpp"

namespace IO
{
	#define NATIVE(name) Value name(VM* vm, int num_args, Value* stack)
	NATIVE(print);
	NATIVE(clock);
	#undef NATIVE
}

#endif
