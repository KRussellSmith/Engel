#include "vm.hpp"
#include "memory.hpp"
#include "value.hpp"
#include "chunk.hpp"
#include <stdlib.h>
#include <string.h>
#include "asprintf.hpp"
#include "native.hpp"
VM::VM()
{
	this->stack         = (Value*)malloc(sizeof(Value)*256);
	this->top           = this->stack;
	this->cap           = 0;
	this->open_upvalues = NULL;
	this->objects       = NULL;
	this->num_frames    = 0;

	init_map(&this->strings);
	init_map(&this->globals);

	this->def_native("print", IO::print);
}
VM::~VM()
{
	puts("freeing vm");
	free(this->stack);
	free_map(&this->globals);
	free_map(&this->strings);
	free_objects(this);
	puts("freed");
}

bool VM::call_val(Value callee, uint64_t num_args)
{
	if (IS_OBJ(callee))
	{
		switch (OBJ_TYPE(callee))
		{
			case OBJ_CLOSURE:
				return this->call(AS_CLOSURE(callee), num_args);
			//case OBJ_FUNCTION:
			//	return this->call(AS_FUNC(callee), num_args);
			case OBJ_NATIVE:
			{
				auto native = AS_NATIVE(callee);
				auto result = native(this, num_args, this->top - num_args);
				this->top -= num_args + 1;
				this->push(result);
				return true;
			}
			default:
				break;
		}
	}
	return false;
}
bool VM::call(Closure* callee, uint64_t num_args)
{
	auto frame = &this->frames[this->num_frames++];
	frame->closure = callee;
	frame->ip      = callee->func->chunk.code;
	frame->slots   = this->top - num_args - 1;
	return true;
}

Upvalue* VM::capture_upvalue(Value* local)
{
	Upvalue* prev = NULL;
	auto  curr = this->open_upvalues;
	while (
		curr != NULL &&
		curr->loc > local)
	{
		prev = curr;
		curr = curr->next;
	}
	if (curr != NULL && curr->loc == local)
	{
		return curr;
	}
	auto created = new_upvalue(this, local);
	created->next = curr;
	if (prev == NULL)
	{
		this->open_upvalues = created;
	}
	else
	{
		prev->next = created;
	}
	return created;
}
void VM::close_upvalues(Value* last)
{
	while (
		this->open_upvalues != NULL &&
		this->open_upvalues->loc >= last)
	{
		auto curr = this->open_upvalues;
		curr->closed = *curr->loc;
		curr->loc = &curr->closed;
		this->open_upvalues = curr->next;
	}
}
void VM::def_native(const char* name, NativeFunc func)
{
	this->push(OBJ_VAL(copy_string(this, name, (int)strlen(name))));
	this->push(OBJ_VAL(new_native(this, func)));
	put_map(&this->globals, AS_STRING(this->top[-2]), this->top[-1]);
	/*printf("defined %s\n", name);
	puts(AS_CSTRING(this->top[-2]));
	Value foo;
	printf("%d\n", get_map(&this->globals, AS_STRING(this->top[-2]), &foo));*/
	this->top -= 2;
}
Value VM::push(Value val)
{
	return *this->top++ = val;
}
Value VM::pop()
{
	return *--this->top;
}
char* VM::to_string(Value val)
{
	char* result = NULL;
	switch (val.type)
	{
		case VALUE_INT:
			asprintf(&result, "%ld", AS_INT(val));
			break;
		case VALUE_REAL:
			asprintf(&result, "%g", AS_REAL(val));
			break;
		case VALUE_BOOL:
			asprintf(&result, "%s", AS_BOOL(val) ? "true" : "false");
			break;
		case VALUE_NULL:
			asprintf(&result, "%s", "null");
			break;
		case VALUE_OBJ:
		{
			Obj* obj = AS_OBJ(val);
			switch (obj->type)
			{
				case OBJ_STRING:
					asprintf(&result, "%s", AS_CSTRING(val));
					break;
				case OBJ_CLOSURE:
				case OBJ_FUNCTION:
					asprintf(&result, "<function>");
					break;
				case OBJ_NATIVE:
					asprintf(&result, "<native>");
					break;
				default:
					asprintf(&result, "UNKOWN OBJECT TYPE (LANGUAGE IMPLEMENTOR SCREWED UP!)");
					break;
			}
		}
		default:
			asprintf(&result, "UNKOWN TYPE (LANGUAGE IMPLEMENTOR SCREWED UP!)");
			break;
	}
	return result;
}
Value VM::to_string_val(Value val)
{
	if (IS_STRING(val)) return val;
	auto chars = this->to_string(val);
	auto result = take_string(this, chars, strlen(chars));
	return OBJ_VAL((Obj*)result);
}
void VM::concat()
{
	ObjString* b = AS_STRING(this->top[-1]);
	ObjString* a = AS_STRING(this->top[-2]);

	int length = a->len + b->len;
	char* chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->len);
	memcpy(chars + a->len, b->chars, b->len);
	chars[length] = '\0';
	this->top -= 2;
	ObjString* result = take_string(this, chars, length);
	*this->top++ = OBJ_VAL((Obj*)result);
}
bool VM::equiv(Value a, Value b)
{
	#define IS_NUM(x) ((x).type == VALUE_INT || (x).type == VALUE_REAL)
	if (a.type != b.type)
	{
		if (IS_NUM(a) && IS_NUM(b))
		{
			if (IS_INT(a))
			{
				return AS_INT(a) == AS_REAL(b);
			}
			return AS_REAL(a) == AS_INT(b);
		}
		return false;
	}
	switch (a.type)
	{
		case VALUE_INT:
			return AS_INT(a) == AS_INT(b);
		case VALUE_REAL:
			return AS_REAL(a) == AS_REAL(b);
		case VALUE_BOOL:
			return AS_BOOL(a) == AS_BOOL(b);
		case VALUE_NULL:
			return true; // Sometimes, giving a special value its own type makes sense…
		case VALUE_OBJ:
			return AS_OBJ(a) == AS_OBJ(b);
		default: break;
	}
	return false;
}
bool VM::is_true(Value val)
{
	if (IS_NULL(val) || (IS_BOOL(val) && AS_BOOL(val) == false))
	{
		return false;
	}
	return true;
}

void VM::run()
{
	auto frame = &this->frames[this->num_frames - 1];
	register uint64_t uleb = 0;
	register uint8_t* ip = frame->ip;
	register uint32_t uint = 0;
	#define READ_BYTE() (*ip++)
	#define INT_LOOP() \
		interpret: \
			switch (READ_BYTE())

	#define DISPATCH() goto interpret
	#define ULEB() \
		do { \
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
	#define UINT() \
		do \
		{  \
			ip += 4; \
			uint = ( \
				((ip[-4] << 24) & 0xFF) | \
				((ip[-3] << 16) & 0xFF) | \
				((ip[-2] <<  8) & 0xFF) | \
				((ip[-1])       & 0xFF)); \
		} while (false)


	#define POP() (*--this->top)
	#define PUSH(val) (*this->top++ = (val))
	#define PEEK(level) (this->top[-1 - (level)])
	#define PUT(level, val) (this->top[-1 - (level)] = (val))

	#define COMP(op) do { \
		auto b = POP(); \
		auto a = POP(); \
		auto a_type = IS_INT(a) ? VALUE_INT : VALUE_REAL; \
		auto b_type = IS_INT(b) ? VALUE_INT : VALUE_REAL; \
		if (a_type == VALUE_REAL) \
		{ \
			if (b_type == VALUE_REAL) \
			{ \
				PUSH(BOOL_VAL(AS_REAL(a) op AS_REAL(b))); \
			} \
			else \
			{ \
				PUSH(BOOL_VAL(AS_REAL(a) op AS_INT(b))); \
			} \
		} \
		else if (b_type == VALUE_REAL) \
		{ \
			PUSH(BOOL_VAL(AS_INT(a) op AS_REAL(b))); \
		} \
		else \
		{ \
			PUSH(BOOL_VAL(AS_INT(a) op AS_INT(b))); \
		} } while (false)

	#define UNARY(op) do { \
		auto a = POP(); \
		auto a_type = IS_INT(a) ? VALUE_INT : VALUE_REAL; \
		if (a_type == VALUE_REAL) \
		{ \
			PUSH(REAL_VAL(op AS_REAL(a))); \
		} \
		else \
		{ \
			PUSH(INT_VAL(op AS_INT(a))); \
		} } while (false)

	#define UNARY_INT(op) do { \
		auto a = POP(); \
		PUSH(INT_VAL(op AS_INT(a))); } while (false)

	#define BINARY(op) do { \
		auto b = POP(); \
		auto a = POP(); \
		auto a_type = IS_INT(a) ? VALUE_INT : VALUE_REAL; \
		auto b_type = IS_INT(b) ? VALUE_INT : VALUE_REAL; \
		if (a_type == VALUE_REAL) \
		{ \
			if (b_type == VALUE_REAL) \
			{ \
				PUSH(REAL_VAL(AS_REAL(a) op AS_REAL(b))); \
			} \
			else \
			{ \
				PUSH(REAL_VAL(AS_REAL(a) op AS_INT(b))); \
			} \
		} \
		else if (b_type == VALUE_REAL) \
		{ \
			PUSH(REAL_VAL(AS_INT(a) op AS_REAL(b))); \
		} \
		else \
		{ \
			PUSH(INT_VAL(AS_INT(a) op AS_INT(b))); \
		} } while (false)

	#define BINARY_INT(op) do { \
		auto b = POP(); \
		auto a = POP(); \
		PUSH(INT_VAL(AS_INT(a) op AS_INT(b))); } while (false)
	#define CONST(x) frame->closure->func->chunk.consts.values[x]
	#define OP(code) case OP_##code
	puts("running");
	INT_LOOP()
	{
		OP(CALL):
		{
			ULEB();
			auto num_args = uleb;
			frame->ip = ip;
			if (!this->call_val(PEEK(num_args), num_args))
			{}
			frame = &this->frames[this->num_frames - 1];
			ip = frame->ip;
			DISPATCH();
		}
		OP(POP):
			POP();
			DISPATCH();
		OP(CONST):
			ULEB();
			PUSH(CONST(uleb));
			DISPATCH();
		OP(NULL): // BOY I LOVE SHOUTIN'
			PUSH(NULL_VAL);
			DISPATCH();
		OP(CONCAT):
			concat();
			DISPATCH();
		OP(TO_STR):
		{
			auto str = this->to_string_val(PEEK(0));
			PUT(0, str);
			DISPATCH();
		}
		OP(NEG):
			UNARY(-);
			DISPATCH();
		OP(ADD):
			if (IS_STRING(PEEK(0)) && IS_STRING(PEEK(1)))
			{
				concat();
			}
			else
			{
				BINARY(+);
			}
			DISPATCH();
		OP(SUB):
			BINARY(-);
			DISPATCH();
		OP(MUL):
			BINARY(*);
			DISPATCH();
		OP(DIV):
			BINARY(/);
			DISPATCH();
		OP(MOD):
			BINARY_INT(%);
			DISPATCH();
		OP(XOR):
			BINARY_INT(^);
			DISPATCH();
		OP(BOR):
			BINARY_INT(|);
			DISPATCH();
		OP(BAND):
			BINARY_INT(&);
			DISPATCH();
		OP(EXP):
			BINARY(-);
			DISPATCH();
		OP(LT):
			COMP(<);
			DISPATCH();
		OP(GT):
			COMP(>);
			DISPATCH();
		OP(LE):
			COMP(<=);
			DISPATCH();
		OP(GE):
			COMP(>=);
			DISPATCH();
		OP(DEF_VAR):
		{
			ULEB();
			auto name = AS_STRING(CONST(uleb));
			put_map(&this->globals, name, PEEK(0));
			POP();
			DISPATCH();
		}
		OP(SET_VAR):
		{
			ULEB();
			auto name = AS_STRING(CONST(uleb));
			if (put_map(&this->globals, name, PEEK(0)))
			{
				rm_map(&this->globals, name);
				puts("Undefined");
				exit(0);
			}
			DISPATCH();
		}
		OP(GET_VAR):
		{
			ULEB();
			auto name = AS_STRING(CONST(uleb));
			Value value;
			if (!get_map(&this->globals, name, &value))
			{
				puts("Undefined variable access");
				exit(0);
			}
			PUSH(value);
			DISPATCH();
		}
		OP(GET_LOCAL):
			ULEB();
			PUSH(frame->slots[uleb]);
			DISPATCH();
		OP(SET_LOCAL):
			ULEB();
			frame->slots[uleb] = PEEK(0);
			DISPATCH();
		OP(GET_UPVAL):
			ULEB();
			PUSH(*frame->closure->upvalues[uleb]->loc);
			DISPATCH();
		OP(SET_UPVAL):
			ULEB();
			*frame->closure->upvalues[uleb]->loc = PEEK(0);
			DISPATCH();
		OP(EQUIV):
			PUT(0, BOOL_VAL(this->equiv(POP(), PEEK(0))));
			DISPATCH();
		OP(GOTO):
			UINT();
			ip = frame->closure->func->chunk.code + uint;
			DISPATCH();
		OP(JMP):
			UINT();
			ip += uint;
			DISPATCH();
		OP(OR):
			UINT();
			if (this->is_true(PEEK(0)))
			{
				ip += uint;
			}
			else
			{
				POP();
			}
			DISPATCH();
		OP(AND):
			UINT();
			if (!this->is_true(PEEK(0)))
			{
				ip += uint;
			}
			else
			{
				POP();
			}
			DISPATCH();
		OP(DUP):
			PUSH(PEEK(0));
			DISPATCH();
		OP(ROT2):
		{
			auto first  = PEEK(0);
			auto second = PEEK(1);

			PUT(0, second);
			PUT(1, first);
			DISPATCH();
		}
		OP(ROT3):
		{
			auto first  = PEEK(0);
			auto second = PEEK(1);
			auto third  = PEEK(2);
			
			PUT(0, second);
			PUT(1, third);
			PUT(2, first);
			DISPATCH();
		}
		OP(ROT4):
		{
			auto first  = PEEK(0);
			auto second = PEEK(1);
			auto third  = PEEK(2);
			auto fourth = PEEK(3);
			
			PUT(0, second);
			PUT(1, third);
			PUT(2, fourth);
			PUT(3, first);
			DISPATCH();
		}
		OP(CLOSURE):
		{
			ULEB();
			auto func = AS_FUNC(CONST(uleb));
			auto closure = new_closure(this, func);
			PUSH(OBJ_VAL(closure));
			for (int i = 0; i < closure->num_upvalues; ++i)
			{
				auto is_local = (bool)READ_BYTE();
				ULEB();
				auto index = uleb;
				if (is_local)
				{
					closure->upvalues[i] = this->capture_upvalue(frame->slots + index);
				}
				else
				{
					closure->upvalues[i] = frame->closure->upvalues[index];
				}
			}
			DISPATCH();
		}
		OP(CLOSE):
			this->close_upvalues(this->top - 1);
			POP();
			DISPATCH();
		OP(RET):
		{
			auto result = POP();
			this->close_upvalues(frame->slots);
			--this->num_frames;
			if (this->num_frames == 0)
			{
				POP();
				return;
			}
			this->top = frame->slots;
			PUSH(result);
			frame = &this->frames[this->num_frames - 1];
			ip = frame->ip;
			DISPATCH();
		}
	}
	
	#undef READ_BYTE
	#undef INT_LOOP
	#undef OP
	#undef DISPATCH
	#undef ULEB
	#undef PUT
	#undef PUSH
	#undef PEEK
	#undef POP
	#undef UINT
	#undef BINARY
	#undef BINARY_INT
	#undef UNARY
	#undef UNARY_INT
}
