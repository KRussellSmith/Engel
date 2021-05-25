#ifndef compiler_header
#define compiler_header
#include <stddef.h>
#include "chunk.hpp"
#include "value.hpp"
#include "node.hpp"
#include "parser.hpp"
class Compiler
{
private:
	Parser* parser;
	Opcode get_bin_op(TokenType type);
	Opcode get_un_op(TokenType type);
	
	uint32_t save_spot();
	void jump(uint32_t spot);
	void go_to(uint32_t spot, uint32_t to);
	
	void emit(uint8_t code);
	void emit_uint(uint32_t code);
	void emit_uleb(uint64_t code);
	void emit_const(Value value);
	
	void visit_binary(Node::Binary* node, Opcode op);
	void visit_unary(Node::Base* node, Opcode op);
	void visit(Node::Base* node);
public:
	Chunk chunk;
	Compiler(Parser* parser);
	void compile();
	~Compiler();
};
#endif
