#include "compiler.hpp"
#include "value.hpp"
#include "chunk.hpp"
#include "parser.hpp"
#include <stdlib.h>
#include <vector>
using namespace Node;

Compiler::Compiler(Parser* parser, VM* vm)
{
	this->parser = parser;
	this->curr_slots = 0;
	this->max_slots  = 0;
	this->vm = vm;
	this->curr_scope = NULL;
}
Compiler::~Compiler()
{
	//free_Chunk(this->chunk());
}

Chunk* Compiler::chunk()
{
	return &this->curr_scope->function->chunk;
}

void Compiler::init_scope(Scope* scope, ScopeType type)
{
	scope->parent = this->curr_scope;
	scope->function = NULL;
	scope->type = type;
	scope->num_locals = 0;
	scope->depth = 0;
	scope->function = new_function(this->vm);
	this->curr_scope = scope;
	this->curr_scope->locals.push_back((Local) {
		.name = (Token) {
			.start = "call",
			.length = 4,
		},
		.depth = 0,
	});
	++this->curr_scope->num_locals;
}
Function* Compiler::end_scope()
{
	this->emit_op(OP_NULL);
	this->emit_op(OP_RET);
	auto func = this->curr_scope->function;
	this->curr_scope = this->curr_scope->parent;
	return func;
}
void Compiler::begin_block()
{
	++this->curr_scope->depth;
}
void Compiler::end_block()
{
	--this->curr_scope->depth;

	while (
		this->curr_scope->num_locals > 0 &&
		this->curr_scope->locals[this->curr_scope->num_locals - 1].depth > this->curr_scope->depth)
	{
		if (this->curr_scope->locals[this->curr_scope->num_locals - 1].captured)
		{
			this->emit_op(OP_CLOSE);
		}
		else
		{
			this->emit_op(OP_POP);
		}
		--this->curr_scope->num_locals;
	}
}

uint64_t Compiler::add_local(Token name)
{
	this->curr_scope->locals.push_back((Local) {
		.name     = name,
		.depth    = this->curr_scope->depth,
		.captured = false,
	});
	return this->curr_scope->num_locals++;
}
uint64_t Compiler::add_upval(Scope* scope, uint64_t index, bool is_local)
{
	auto num_upvals = scope->function->num_upvalues;
	for (int i = 0; i < num_upvals; ++i)
	{
		auto upval = &scope->upvalues[i];
		if (upval->index == index && upval->is_local == is_local)
		{
			return i;
		}
	}
	scope->upvalues.push_back((Upval)
	{
		.index    = index,
		.is_local = is_local,
	});
	return scope->function->num_upvalues++;
}
int64_t Compiler::resolve_local(Scope* scope, Token* name)
{
	for (int i = scope->num_locals - 1; i >= 0; --i)
	{
		Local* local = &scope->locals[i];
		if (equal_idents(name, &local->name))
		{
			return i;
		}
	}
	return -1;
}
int64_t Compiler::resolve_upval(Scope* scope, Token* name)
{
	if (scope->parent == NULL)
	{
		return -1;
	}
	auto local = this->resolve_local(scope->parent, name);
	if (local != -1)
	{
		scope->parent->locals[local].captured = true;
		return add_upval(scope, (uint64_t)local, true);
	}
	auto upval = this->resolve_upval(scope->parent, name);
	if (upval != -1)
	{
		return this->add_upval(scope, (uint64_t)upval, false);
	}
	return -1;
}
const int Compiler::stack_effect[]
{
	#define OP(_, effect) effect
	#include "opcode.txt"
	#undef OP
};
void Compiler::mod_stack(int stack_effect)
{
	this->curr_slots += stack_effect;
	if (this->curr_slots > this->max_slots)
	{
		this->max_slots = this->curr_slots;
	}
}

uint32_t Compiler::save_spot()
{
	return this->chunk()->len;
}
void Compiler::jump(uint32_t spot)
{
	auto chunk = this->chunk();
	auto jump = (uint32_t)(chunk->len - spot - 4);
	chunk->code[spot + 0] = (jump >> 24) & 0xFF;
	chunk->code[spot + 1] = (jump >> 16) & 0xFF;
	chunk->code[spot + 2] = (jump >> 8)  & 0xFF;
	chunk->code[spot + 3] = (jump)       & 0xFF;
}
uint32_t Compiler::prep_jump(Opcode op)
{
	this->emit_op(op);
	auto result = this->save_spot();
	this->emit_uint(0);
	return result;
}
void Compiler::go_to(uint32_t spot, uint32_t to)
{
	auto chunk = this->chunk();
	chunk->code[spot + 0] = (to >> 24) & 0xFF;
	chunk->code[spot + 1] = (to >> 16) & 0xFF;
	chunk->code[spot + 2] = (to >> 8)  & 0xFF;
	chunk->code[spot + 3] = (to)       & 0xFF;
}
Opcode Compiler::get_un_op(TokenType type)
{
	switch (type)
	{
		case TOKEN_SUB:
			return OP_NEG;
		case TOKEN_TIL:
			return OP_BNOT;
		case TOKEN_BANG:
			return OP_NOT;
		default:
			return OP_NEG;
	}
	return OP_NEG;
}
Opcode Compiler::get_bin_op(TokenType type)
{
	#define CONV(token, op) \
		case TOKEN_##token: \
			return OP_##op

	switch (type)
	{
		// Arithmetic and bitwise:
		CONV (ADD,    ADD);
		CONV (SUB,    SUB);
		CONV (MUL,    MUL);
		CONV (DIV,    DIV);
		CONV (EXP,    EXP);
		CONV (MOD,    MOD);
		CONV (LSHIFT, LSHIFT);
		CONV (RSHIFT, RSHIFT);
		CONV (BOR,    BOR);
		CONV (BAND,   BAND);
		CONV (TIL,    XOR);
		
		// Inplace:
		CONV (ADD_SET,    I_ADD);
		CONV (SUB_SET,    I_SUB);
		CONV (MUL_SET,    I_MUL);
		CONV (DIV_SET,    I_DIV);
		CONV (MOD_SET,    I_MOD);
		CONV (EXP_SET,    I_EXP);
		CONV (LSHIFT_SET, I_LSHIFT);
		CONV (RSHIFT_SET, I_RSHIFT);
		CONV (BOR_SET,    I_BOR);
		CONV (BAND_SET,   I_BAND);
		CONV (TIL_SET,    I_XOR);
		
		// Comparision:
		CONV (LT,      LT);
		CONV (GT,      GT);
		CONV (LE,      LE);
		CONV (GE,      GE);
		CONV (BANG_EQ, NOT_EQUIV);
		CONV (EQUIV,   EQUIV);
		
		// Control:
		CONV (AMP_AMP,  AND);
		CONV (PIP_PIP,  OR);
		CONV (COAL,     COAL);
		CONV (QUESTION, OPTIONAL);

		// Prevent compiler warnings:
		default: break;
	}
	return OP_ADD;
	#undef CONV
}

void Compiler::emit(uint8_t code)
{
	write_Chunk(this->chunk(), code);
}
void Compiler::emit_op(Opcode op)
{
	this->emit(op);
	this->mod_stack(this->stack_effect[op]);
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
void Compiler::add_const(Value value)
{
	push_ValueArray(&this->chunk()->consts, value);
	this->emit_uleb(this->chunk()->consts.len - 1);
}
void Compiler::emit_const(Value value)
{
	push_ValueArray(&this->chunk()->consts, value);
	this->emit_op(OP_CONST);
	this->emit_uleb(this->chunk()->consts.len - 1);
}
void Compiler::visit_binary(Binary* node, Opcode op)
{
	this->visit(node->left);
	this->visit(node->right);
	this->emit_op(op);
}
void Compiler::visit_unary(Unary* node, Opcode op)
{
	this->visit(node->child);
	this->emit_op(op);
}
bool Compiler::equal_idents(Token* a, Token* b)
{
	if (a->length != b->length)
	{
		return false;
	}
	return memcmp(a->start, b->start, a->length) == 0;
}
void Compiler::visit(Base* node)
{
	switch (node->type)
	{
		case NODE_EXPR:
		{
			auto expr = (Expr*)node;
			this->visit(expr->child);
			this->emit_op(OP_POP);
			break;
		}
		case NODE_GROUP:
		{
			this->visit(((Group*)node)->child);
			break;
		}
		case NODE_UNARY:
		{
			auto unary = (Unary*)node;
			this->visit_unary(unary, this->get_un_op(unary->op));
			break;
		}
		case NODE_COND:
		{
			auto cond = (Cond*)node;
			this->visit(cond->left);
			auto jmp = this->prep_jump(this->get_bin_op(cond->op));
			this->visit(cond->right);
			this->jump(jmp);
			break;
		}
		case NODE_BINARY:
		{
			auto binary = (Binary*)node;
			this->visit_binary(binary, this->get_bin_op(binary->op));
			break;
		}
		case NODE_CONST:
		{
			auto con = (Const*)node;
			auto value = con->value;
			this->emit_const(value);
			break;
		}
		case NODE_INTERP:
		{
			auto interps = (StringInterp*)node;
			auto first = true;
			for (auto interp = interps->list.begin(); interp != interps->list.end(); ++interp)
			{
				this->emit_const((*interp)->chars.value);
				this->visit((*interp)->value);
				this->emit(OP_TO_STR);
				this->emit(OP_CONCAT);
				if (!first)
				{
					this->emit(OP_CONCAT);
				}
				else first = false;
			}
			this->emit_const(interps->cap.value);
			this->emit(OP_CONCAT);
			break;
		}
		case NODE_COMP:
		{
			auto exit = this->save_spot(); // Simply shutting up the linter; this will be redefined.
			auto comp = (Comparisons*)node;
			this->visit(comp->primer);
			std::vector<uint32_t> jumps;
			auto chained = comp->list.size() > 1;
			for (auto curr = comp->list.begin(); curr != comp->list.end(); ++curr)
			{
				if (std::next(curr) == comp->list.end())
				{
					this->visit((*curr)->value);
					this->emit(this->get_bin_op((*curr)->type));
					if (chained)
					{
						exit = this->prep_jump(OP_JMP);
					}
				}
				else
				{
					this->visit((*curr)->value);
					this->emit(OP_DUP);
					this->emit(OP_ROT3);
					this->emit(this->get_bin_op((*curr)->type));
					jumps.push_back(this->prep_jump(OP_AND));
				}
			}
			// The result of these operations is going to be on top,
			// So this will switch it with the second-place value, then pop that:
			for (auto jump : jumps)
			{
				this->jump(jump);
			}
			if (chained)
			{
				this->emit(OP_ROT2);
				this->emit(OP_POP);
				this->jump(exit);
			}
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
					jumps.push_back(this->prep_jump(OP_OR));
				}
				auto nomatch = this->prep_jump(OP_JMP);
				for (auto jump = jumps.begin(); jump != jumps.end(); ++jump)
				{
					this->jump(*jump);
				}
				this->emit(OP_POP);
				this->emit(OP_POP);
				this->visit((*mcase)->then);
				if (match->other != NULL || std::next(mcase) == match->cases.end())
				{
					exits.push_back(this->prep_jump(OP_JMP));
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
		case NODE_DEC:
		{
			auto decs = (Declarations*)node;
			for (auto dec = decs->list.begin(); dec != decs->list.end(); ++dec)
			{
				this->visit((*dec)->value);
				auto name = &(*dec)->name;
				if (this->curr_scope->depth == 0)
				{
					this->emit_op(OP_DEF_VAR);
					this->add_const(OBJ_VAL((Obj*)copy_string(vm, name->start, name->length)));
				}
				else
				{
					for (auto &local : this->curr_scope->locals)
					{
						if (
							local.depth != -1 &&
							local.depth < this->curr_scope->depth)
						{
							break;
						}
						if (this->equal_idents(name, &local.name))
						{
							puts("Already declared in scope.");
						}
					}
					this->add_local(*name);
				}
			}
			break;
		}
		case NODE_GET:
		{
			auto get = (Get*)node;
			auto name = &get->name;
			auto index = resolve_local(this->curr_scope, name);
			if (index != -1)
			{
				this->emit_op(OP_GET_LOCAL);
				this->emit_uleb(index);
			}
			else if ((index = resolve_upval(this->curr_scope, name)) != -1)
			{
				this->emit_op(OP_GET_UPVAL);
				this->emit_uleb(index);
			}
			else
			{
				this->emit_op(OP_GET_VAR);
				this->add_const(OBJ_VAL((Obj*)copy_string(vm, name->start, name->length)));
			}
			break;
		}
		case NODE_SET:
		{
			auto set = (Set*)node;
			auto name = &((Get*)set->left)->name;
			auto right = set->right;
			this->visit(right);
			auto index = resolve_local(this->curr_scope, name);
			if (index != -1)
			{
				this->emit_op(OP_SET_LOCAL);
				this->emit_uleb(index);
			}
			else if ((index = resolve_upval(this->curr_scope, name)) != -1)
			{
				this->emit_op(OP_SET_UPVAL);
				this->emit_uleb(index);
			}
			else
			{
				this->emit_op(OP_SET_VAR);
				this->add_const(OBJ_VAL((Obj*)copy_string(vm, name->start, name->length)));
			}
			break;
		}
		case NODE_BLOCK:
		{
			auto block = (Block*)node;
			this->begin_block();
			for (auto &node : block->list)
			{
				this->visit(node);
			}
			this->end_block();
			break;
		}
		case NODE_FUNCBODY:
		{
			auto block = (FuncBody*)node;
			for (auto &node : block->list)
			{
				this->visit(node);
			}
			break;
		}
		case NODE_FUNC:
		{
			auto func = (Func*)node;
			Scope scope;
			this->init_scope(&scope, SCOPE_FUNC);
			this->begin_block();
			/*this->add_local((Token) {
				.start = "call", .length = 4,
			});*/
			for (auto &arg : func->args)
			{
				switch (arg->type)
				{
					case NODE_GET:
						this->add_local(((Get*)arg)->name);
						break;
					default: puts("Invalid argument type.");
				}
			}
			this->visit(func->body);
			auto result = this->end_scope();
			result->arity = func->args.size();
			this->emit_op(OP_CLOSURE);
			this->add_const(OBJ_VAL((Obj*)result));
			for (auto i = 0; i < result->num_upvalues; ++i)
			{
				this->emit(scope.upvalues[i].is_local ? 1 : 0);
				this->emit_uleb(scope.upvalues[i].index);
			}
			break;
		}
		case NODE_RETURN:
		{
			auto ret = (Return*)node;
			this->visit(ret->expr);
			this->emit(OP_RET);
			break;
		}
		case NODE_FUNCCALL:
		{
			auto call = (FuncCall*)node;
			this->visit(call->callee);
			for (auto &arg : call->args)
			{
				this->visit(arg);
			}
			this->emit_op(OP_CALL);
			this->emit_uleb(call->args.size());
			break;
		}
		case NODE_IF:
		{
			auto ifelse = (If*)node;
			auto has_else = ifelse->other != NULL;
			this->visit(ifelse->cond);
			auto if_false = this->prep_jump(OP_AND);
			this->visit(ifelse->then);
			if (has_else)
			{
				auto if_true = this->prep_jump(OP_JMP);
				this->jump(if_false);
				this->emit_op(OP_POP);
				this->visit(ifelse->other);
				this->jump(if_true);
			}
			else
			{
				this->jump(if_false);
				this->emit_op(OP_POP);
			}
			break;
		}
		case NODE_WHILE:
		{
			auto while_ = (While*)node;
			auto has_else = while_->other != NULL;
			if (has_else)
			{
				// while/else loops are essentially this:
				// if condition
				// {
				//		do
				//		{
				//			...
				//		} while condition
				//	}
				//	else
				//	{
				//		...
				//	}
				//	This is the most sensible way I've found to compile it.
				this->visit(while_->cond);
				auto initial = this->prep_jump(OP_AND);
				auto start   = this->save_spot();
				this->visit(while_->then);
				this->visit(while_->cond);
				auto big_break = this->prep_jump(OP_AND);
				this->go_to(this->prep_jump(OP_GOTO), start);
				this->jump(initial);
				this->emit_op(OP_POP);
				this->visit(while_->other);
				auto exit_else = this->prep_jump(OP_JMP);
				this->jump(big_break);
				this->emit_op(OP_POP);
				this->jump(exit_else);
			}
			else
			{
				auto start = this->save_spot();
				this->visit(while_->cond);
				auto big_break = this->prep_jump(OP_AND);
				this->visit(while_->then);
				this->go_to(this->prep_jump(OP_GOTO), start);
				this->jump(big_break);
				this->emit_op(OP_POP);
			}
		}
	}
}
Function* Compiler::compile()
{
	Scope scope;
	this->init_scope(&scope, SCOPE_SCRIPT);
	init_Chunk(this->chunk());
	for (;;)
	{
		auto node = this->parser->parse();
		if (node->type == NODE_FIN)
		{
			break;
		}
		this->visit(node);
		destroy(node);
	}
	auto result = this->end_scope();
	return result;
}
