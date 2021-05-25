#ifndef vm_header
#define vm_header
#include "chunk.hpp"
#include "value.hpp"
#include <stddef.h>
class VM
{
public:
	Value*   stack;
	uint8_t* ip;
	int      top;
	int      cap;
	VM();
	~VM();
	void run(Chunk* chunk);
};
#define POP(vm) ((vm)->stack[--((vm)->top)])
#define PUSH(vm, x) ((vm)->stack[(vm)->top++] = x)
#define PEEK(vm, level) ((vm)->stack[(vm)->top - 1 - level])
#endif
