#ifndef vm_header
#define vm_header
#include "chunk.hpp"
#include "value.hpp"
#include <stddef.h>
#define MAX_FRAMES 64
typedef struct
{
	Closure* closure;
	uint8_t*  ip;
	Value*    slots;
} CallFrame;
class VM
{
public:
	static bool  equiv(Value a, Value b);
	static bool  is_true(Value val);
	char* to_string(Value val);
	Value to_string_val(Value val);

	CallFrame frames[MAX_FRAMES];
	int       num_frames;
	Value*    stack;
	Value*    top;
	int       cap;
	Upvalue*  open_upvalues;
	Obj*      objects;
	Map       strings;
	Map       globals;
	Map       const_table;
	Value     push(Value val);
	Value     pop();
	void      concat();

	bool      call_val(Value callee, uint64_t num_args);
	bool      call(Closure* callee, uint64_t num_args);
	Upvalue*  capture_upvalue(Value* local);
	void      close_upvalues(Value* last);

	void      def_native(const char* name, NativeFunc func);

	VM();
	~VM();
	void run();
};
//#define POP(vm) ((vm)->stack[--((vm)->top)])
//#define PUSH(vm, x) ((vm)->stack[(vm)->top++] = x)
//#define PEEK(vm, level) ((vm)->stack[(vm)->top - 1 - level])
#endif
