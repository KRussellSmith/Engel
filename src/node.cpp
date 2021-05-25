#include "lexer.hpp"
#include "node.hpp"
#include "value.hpp"
#include <stdio.h>
using namespace Node;

void Node::destroy(Base* node)
{
	if (node == NULL)
	{
		return;
	}
	#define HANDLE(type, Class) \
		case NODE_##type: \
			/*puts("deletin' " #type);*/ \
			delete (Class*)node; \
			break
	switch (node->type)
	{
		HANDLE (EXPR,     Expr);
		HANDLE (GROUP,    Group);
		HANDLE (UNARY,    Unary);
		HANDLE (COND,     Cond);
		HANDLE (BINARY,   Binary);
		HANDLE (CONST,    Const);
		HANDLE (INTERP,   StringInterp);
		HANDLE (COMP,     Comparisons);
		HANDLE (MATCH,    Match);
		HANDLE (DEC,      Declarations);
		HANDLE (SET,      Set);
		HANDLE (GET,      Get);
		HANDLE (BLOCK,    Block);
		HANDLE (FUNC,     Func);
		HANDLE (FUNCBODY, FuncBody);
		HANDLE (RETURN,   Return);
		HANDLE (FUNCCALL, FuncCall);
		HANDLE (IF,       If);
		HANDLE (WHILE,    While);
		HANDLE (FIN,      Base);
	}
	#undef HANDLE
}

Base::Base(NodeType type)
{
	this->type = type;
}

Const::Const(Token* token) : Base(NODE_CONST)
{
	this->value = token->value;
}

Expr::Expr(Base* child) : Base(NODE_EXPR)
{
	this->child = child;
}
Expr::~Expr()
{
	destroy(this->child);
}

Group::Group(Base* child) : Base(NODE_GROUP)
{
	this->child = child;
}
Group::~Group()
{
	destroy(this->child); // :'(
}

Unary::Unary(TokenType op, Base* child) : Base(NODE_UNARY)
{
	this->op    = op;
	this->child = child;
}
Unary::~Unary()
{
	destroy(this->child);
}

Cond::Cond(
	TokenType op,
	Base* left, Base* right) : Base(NODE_COND)
{
	this->op    = op;
	this->left  = left;
	this->right = right;
}
Cond::~Cond()
{
	destroy(this->left);
	destroy(this->right);
}

Binary::Binary(
	TokenType op,
	Base* left, Base* right) : Base(NODE_BINARY)
{
	this->op    = op;
	this->left  = left;
	this->right = right;
}
Binary::~Binary()
{
	destroy(this->left);
	destroy(this->right);
}

Comparison::Comparison(TokenType type, Base* value)
{
	this->type  = type;
	this->value = value;
}
Comparison::~Comparison()
{
	destroy(this->value);
}
Comparisons::Comparisons(Base* primer) : Base(NODE_COMP)
{
	this->primer = primer;
}
Comparisons::~Comparisons()
{
	destroy(this->primer);
	for (auto comp = this->list.begin(); comp != this->list.end(); ++comp)
	{
		delete *comp;
	}
	this->list.clear();
}

Case::Case()
{
	this->then = NULL;
}
Case::~Case()
{
	for (auto node = this->checks.begin(); node != this->checks.end(); ++node)
	{
		destroy(*node);
	}
	this->checks.clear();
	destroy(this->then);
}
Match::Match() : Base(NODE_MATCH)
{
	this->comp  = NULL;
	this->other = NULL;
}
Match::~Match()
{
	for (auto cas = this->cases.begin(); cas != this->cases.end(); ++cas)
	{
		delete *cas;
	}
	this->cases.clear();
	destroy(this->comp);
	destroy(this->other);
}

Declaration::Declaration(Token name, Base* value, bool mut)
{
	this->name  = name;
	this->value = value;
	this->mut   = mut;
}
Declaration::~Declaration()
{
	destroy(this->value);
}
Declarations::Declarations() : Base(NODE_DEC)
{}
Declarations::~Declarations()
{
	for (auto dec = this->list.begin(); dec != this->list.end(); ++dec)
	{
		delete *dec;
	}
	this->list.clear();
}

Get::Get(Token name) : Base(NODE_GET)
{
	this->name = name;
}
Set::Set(
	TokenType op,
	Base* left, Base* right) : Base(NODE_SET)
{
	this->op    = op;
	this->left  = left;
	this->right = right;
}
Set::~Set()
{
	destroy(this->left);
	destroy(this->right);
}
Interpolation::Interpolation(Token chars, Base* value)
{
	this->chars = chars;
	this->value = value;
}
Interpolation::~Interpolation()
{
	destroy(this->value);
}
StringInterp::StringInterp() : Base(NODE_INTERP)
{}
StringInterp::~StringInterp()
{
	for (auto item = this->list.begin(); item != this->list.end(); ++item)
	{
		delete *item;
	}
	this->list.clear();
}

Block::Block() : Base(NODE_BLOCK)
{}
Block::~Block()
{
	for (auto item = this->list.begin(); item != this->list.end(); ++item)
	{
		destroy(*item);
	}
	this->list.clear();
}

Func::Func() : Base(NODE_FUNC)
{}
Func::~Func()
{
	for (auto item = this->args.begin(); item != this->args.end(); ++item)
	{
		destroy(*item);
	}
	destroy(this->body);
}

FuncBody::FuncBody() : Base(NODE_FUNCBODY)
{}
FuncBody::~FuncBody()
{
	for (auto item = this->list.begin(); item != this->list.end(); ++item)
	{
		destroy(*item);
	}
	this->list.clear();
}

Return::Return(Base* expr) : Base(NODE_RETURN)
{
	this->expr = expr;
}
Return::~Return()
{
	destroy(this->expr);
}

FuncCall::FuncCall(Base* callee) : Base(NODE_FUNCCALL)
{
	this->callee = callee;
}
FuncCall::~FuncCall()
{
	for (auto item = this->args.begin(); item != this->args.end(); ++item)
	{
		destroy(*item);
	}
	destroy(this->callee);
}
If::If(Base* cond, Base* then, Base* other) : Base(NODE_IF)
{
	this->cond  = cond;
	this->then  = then;
	this->other = other;
}
If::~If()
{
	destroy(this->cond);
	destroy(this->then);
	destroy(this->other);
}

While::While(Base* cond, Base* then, Base* other) : Base(NODE_WHILE)
{
	this->cond  = cond;
	this->then  = then;
	this->other = other;
}
While::~While()
{
	destroy(this->cond);
	destroy(this->then);
	destroy(this->other);
}
