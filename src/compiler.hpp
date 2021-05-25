#ifndef compiler_header
#define compiler_header
#include <stddef.h>
#include "chunk.hpp"
#include "value.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "vm.hpp"

typedef enum
{
	SCOPE_FUNC,
	SCOPE_SCRIPT,
} ScopeType;

typedef struct
{
	Token name;
	int depth;
	bool captured;
} Local;
typedef struct
{
	uint64_t index;
	bool     is_local;
} Upval;
typedef struct Loop
{
	std::vector<uint32_t> continues;
	std::vector<uint32_t> breaks;
	uint32_t start;
	int depth;
	struct Loop* parent;
} Loop;
typedef struct Scope
{
	struct Scope* parent;
	Function* function;
	ScopeType type;
	std::vector<Local> locals;
	std::vector<Upval> upvalues;
	int num_locals;
	int depth;
} Scope;
class Compiler
{
private:
	Scope* curr_scope;
	Parser* parser;
	int max_slots;
	int curr_slots;
	static const int stack_effect[];
	VM* vm;
	Opcode get_bin_op(TokenType type);
	Opcode get_un_op(TokenType type);
	
	Chunk* chunk();

	void init_scope(Scope* scope, ScopeType type);
	Function* end_scope();
	void begin_block();
	void end_block();
	bool equal_idents(Token* a, Token* b);

	uint64_t add_local(Token name);
	uint64_t add_upval(Scope* scope, uint64_t index, bool is_local);
	int64_t resolve_local(Scope* scope, Token* name);
	int64_t resolve_upval(Scope* scope, Token* name);

	uint32_t save_spot();
	void jump(uint32_t spot);
	uint32_t prep_jump(Opcode op);
	void go_to(uint32_t spot, uint32_t to);
		
	void emit(uint8_t code);
	void emit_op(Opcode op);
	void emit_uint(uint32_t code);
	void emit_uleb(uint64_t code);
	void add_const(Value value);
	void emit_const(Value value);
	void visit_binary(Node::Binary* node, Opcode op);
	void visit_unary(Node::Unary* node, Opcode op);
	void visit(Node::Base* node);
	void mod_stack(int stack_effect);
public:
	Compiler(Parser* parser, VM* vm);
	Function* compile();
	~Compiler();
};
#endif
