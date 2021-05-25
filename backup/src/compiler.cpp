#include "compiler.hpp"
#include "value.hpp"
#include "chunk.hpp"
#include "parser.hpp"
#include <stdlib.h>
#include <vector>
using namespace Node;
Compiler::Compiler(Parser* parser)
{
	this->parser = parser;
	init_Chunk(&this->chunk);
}
Compiler::~Compiler()
{
	free_Chunk(&this->chunk);
}

uint32_t Compiler::save_spot()
{
	return this->chunk.len;
}
void Compiler::jump(uint32_t spot)
{
	auto chunk = &this->chunk;
	auto jump = (uint32_t)(chunk->len - spot - 4);
	chunk->code[spot + 0] = (jump >> 24) & 0xFF;
	chunk->code[spot + 1] = (jump >> 16) & 0xFF;
	chunk->code[spot + 2] = (jump >> 8)  & 0xFF;
	chunk->code[spot + 3] = (jump)       & 0xFF;
}
void Compiler::go_to(uint32_t spot, uint32_t to)
{
	auto chunk = &this->chunk;
	chunk->code[spot + 0] = (to >> 24) & 0xFF;
	chunk->code[spot + 1] = (to >> 16) & 0xFF;
	chunk->code[spot + 2] = (to >> 8)  & 0xFF;
	chunk->code[spot + 3] = (to)       & 0xFF;
}

Opcode Compiler::get_bin_op(TokenType type)
{
	switch (type)
	{
		// Arithmetic and bitwise:
		case TOKEN_ADD:
			return OP_ADD;
		case TOKEN_SUB:
			return OP_SUB;
		case TOKEN_MUL:
			return OP_MUL;
		case TOKEN_MOD:
			return OP_MOD;
		case TOKEN_DIV:
			return OP_DIV;
		case TOKEN_EXP:
			return OP_EXP;
		case TOKEN_LSHIFT:
			return OP_LSHIFT;
		case TOKEN_RSHIFT:
			return OP_RSHIFT;
		case TOKEN_BOR:
			return OP_BOR;
		case TOKEN_BAND:
			return OP_BAND;
		case TOKEN_TIL:
			return OP_XOR;
		
		// Inplace:
		case TOKEN_ADD_SET:
			return OP_I_ADD;
		case TOKEN_SUB_SET:
			return OP_I_SUB;
		case TOKEN_MUL_SET:
			return OP_I_MUL;
		case TOKEN_MOD_SET:
			return OP_I_MOD;
		case TOKEN_DIV_SET:
			return OP_I_DIV;
		case TOKEN_EXP_SET:
			return OP_I_EXP;
		case TOKEN_LSHIFT_SET:
			return OP_I_LSHIFT;
		case TOKEN_RSHIFT_SET:
			return OP_I_RSHIFT;
		case TOKEN_BOR_SET:
			return OP_I_BOR;
		case TOKEN_BAND_SET:
			return OP_I_BAND;
		case TOKEN_TIL_SET:
			return OP_I_XOR;
		
		// Comparision:
		case TOKEN_LT:
			return OP_LT;
		case TOKEN_GT:
			return OP_GT;
		case TOKEN_LE:
			return OP_LE;
		case TOKEN_GE:
			return OP_GE;
		case TOKEN_BANG_EQ:
			return OP_NOT_EQUIV;
		case TOKEN_EQUIV:
			return OP_EQUIV;
	}
	return OP_ADD;
}
void Compiler::emit(uint8_t code)
{
	write_Chunk(&this->chunk, code);
}
void Compiler::emit_uint(uint32_t code)
{
	this->emit((code >> 24) & 0xFF);
	this->emit((code >> 16) & 0xFF);
	this->emit((code >>  8) & 0xFF);
	this->emit((code)       & 0xFF);
}
void Compiler::emit_uleb(uint64_t code)
{
	do
	{
		uint8_t x = code & 0x7F;
		code >>= 7;
		if (code != 0)
		{
			x |= 0x80;
		}
		this->emit(x);
	} while (code != 0);
}
void Compiler::emit_const(Value value)
{
	push_ValueArray(&this->chunk.consts, value);
	this->emit(OP_CONST);
	this->emit_uleb(this->chunk.consts.len - 1);
}
void Compiler::visit_binary(Binary* node, Opcode op)
{
	this->visit(node->left);
	this->visit(node->right);
	this->emit(op);
}
void Compiler::visit(Base* node)
{
	switch (node->type)
	{
		case NODE_BINARY:
		{
			Binary* binary = (Binary*)node;
			this->visit_binary(binary, this->get_bin_op(binary->op));
			break;
		}
		case NODE_INT:
		{
			auto con = (Const*)node;
			auto value = strtol(con->value, NULL, 10);
			this->emit_const(INT_VAL(value));
			break;
		}
		case NODE_MATCH:
		{
			auto match = (Match*)node;
			this->visit(match->comp);
			std::vector<uint32_t> exits;
			for (auto mcase = match->cases.begin(); mcase != match->cases.end(); ++mcase)
			{
				std::vector<uint32_t> jumps;
				for (auto check = (*mcase)->checks.begin(); check != (*mcase)->checks.end(); ++check)
				{
					this->emit(OP_DUP);
					this->visit(*check);
					this->emit(OP_EQUIV);
					this->emit(OP_OR);
					jumps.push_back(this->save_spot());
					this->emit_uint(0);
				}
				this->emit(OP_JMP);
				auto nomatch = this->save_spot();
				this->emit_uint(0);
				for (auto jump = jumps.begin(); jump != jumps.end(); ++jump)
				{
					this->jump(*jump);
				}
				this->emit(OP_POP);
				this->emit(OP_POP);
				this->visit((*mcase)->then);
				if (match->other != NULL || *mcase != match->cases[match->cases.size() - 1])
				{
					this->emit(OP_JMP);
					exits.push_back(this->save_spot());
					this->emit_uint(0);
				}
				this->jump(nomatch);
			}
			this->emit(OP_POP);
			if (match->other != NULL)
			{
				this->visit(match->other);
			}
			for (auto exit = exits.begin(); exit != exits.end(); ++exit)
			{
				this->jump(*exit);
			}
			break;
		}
	}
}
void Compiler::compile()
{
	for (;;)
	{
		auto node = this->parser->parse();
		if (node == NULL)
		{
			break;
		}
		this->visit(node);
		delete node;
	}
}
