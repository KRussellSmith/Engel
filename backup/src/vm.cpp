#include "vm.hpp"
#include "memory.hpp"
#include "value.hpp"
#include "chunk.hpp"
#include <stdlib.h>
VM::VM()
{
	this->stack = (Value*)malloc(sizeof(Value)*256);
	this->top   = 0;
	this->cap   = 0;
	this->ip    = NULL;
}
VM::~VM()
{
	free(this->stack);
}

void VM::run(Chunk* chunk)
{
	register uint64_t uleb = 0;
	register uint8_t* ip = chunk->code;
	#define READ_BYTE() (*ip++)
	#define INT_LOOP() \
		interpret:      \
			switch (READ_BYTE())

	#define DISPATCH() goto interpret
	#define ULEB()  \
		do {         \
			uleb = 0; \
			uint8_t shift = 0; \
			uint8_t val; \
			for (;;) \
			{ \
				val = READ_BYTE(); \
				uleb |= (uint64_t)(val & 0x7F) << shift; \
				if ((val & 0x80) == 0) \
				{ \
					break; \
				} \
				shift += 7; \
			} \
		} while (false)


	INT_LOOP()
	{
		case OP_CONST:
			ULEB();
			PUSH(this, chunk->consts.values[uleb]);
			DISPATCH();
		case OP_ADD:
		{
			auto b = POP(this);
			auto a = POP(this);
			auto a_type = IS_INT(a) ? VALUE_INT : VALUE_REAL;
			auto b_type = IS_INT(b) ? VALUE_INT : VALUE_REAL;
			if (a_type == VALUE_REAL)
			{
				if (b_type == VALUE_REAL)
				{
					PUSH(this, REAL_VAL(AS_REAL(a) + AS_REAL(b)));
				}
				else
				{
					PUSH(this, REAL_VAL(AS_REAL(a) + AS_INT(b)));
				}
			}
			else if (b_type == VALUE_REAL)
			{
				PUSH(this, REAL_VAL(AS_INT(a) + AS_REAL(b)));
			}
			else
			{
				PUSH(this, INT_VAL(AS_INT(a) + AS_INT(b)));
			}
			DISPATCH();
		}
	}
	
	#undef READ_BYTE
	#undef INT_LOOP
	#undef DISPATCH
	#undef ULEB
}
