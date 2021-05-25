#ifndef chunk_header
#define chunk_header
#include <stddef.h>
#include <stdint.h>
#include "value.hpp"
typedef enum
{
	OP_CONST,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_MOD,
	OP_DIV,
	OP_EXP,
	OP_LSHIFT,
	OP_RSHIFT,
	OP_BOR,
	OP_BAND,
	OP_XOR,
	OP_NEG,
	OP_I_ADD,
	OP_I_SUB,
	OP_I_MUL,
	OP_I_MOD,
	OP_I_DIV,
	OP_I_EXP,
	OP_I_LSHIFT,
	OP_I_RSHIFT,
	OP_I_BOR,
	OP_I_BAND,
	OP_I_XOR,
	OP_LT,
	OP_GT,
	OP_LE,
	OP_GE,
	OP_NOT_EQUIV,
	OP_EQUIV,
	
	OP_POP,
	OP_ROT_2,
	OP_ROT_3,
	OP_ROT_4,
	OP_DUP,
	
	OP_JMP,
	OP_OR,
	OP_AND,
} Opcode;
typedef struct
{
	long len, cap;
	uint8_t* code;
	ValueArray consts;
} Chunk;
void init_Chunk(Chunk*  chunk);
void write_Chunk(Chunk* chunk, uint8_t code);
void free_Chunk(Chunk*  chunk);
#endif
