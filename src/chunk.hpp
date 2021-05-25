#ifndef chunk_header
#define chunk_header
#include <stddef.h>
#include <stdint.h>
typedef struct
{
	int len;
	int cap;
	struct Value* values;
} ValueArray;

void init_ValueArray(ValueArray* array);
void push_ValueArray(ValueArray* array, struct Value value);
void free_ValueArray(ValueArray* array);

typedef enum
{
	#define OP(name, _) OP_##name
	#include "opcode.txt"
	#undef OP
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
