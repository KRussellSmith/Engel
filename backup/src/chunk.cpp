#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp"
#include <stdlib.h>
#include <stddef.h>
void init_Chunk(Chunk* chunk)
{
	chunk->len  = 0;
	chunk->cap  = 0;
	chunk->code = NULL;
	init_ValueArray(&chunk->consts);
}
void write_Chunk(Chunk* chunk, uint8_t code)
{
	if (chunk->len >= chunk->cap)
	{
		int old = chunk->cap;
		chunk->cap = GROW(old);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, old, chunk->cap); 
	}
	chunk->code[chunk->len] = code;
	++chunk->len;
}
void free_Chunk(Chunk* chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
	free_ValueArray(&chunk->consts);
	init_Chunk(chunk);
}

