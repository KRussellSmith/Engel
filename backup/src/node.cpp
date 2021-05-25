#include "lexer.hpp"
#include "node.hpp"
using namespace Node;
Const::Const(NodeType type, Token* value)
{
	this->type   = type;
	this->value  = value->start;
	this->length = value->length;
}

Binary::Binary(
	TokenType op,
	Base* left, Base* right)
{
	this->type  = NODE_BINARY;
	this->op    = op;
	this->left  = left;
	this->right = right;
}
Binary::~Binary()
{
	delete this->left;
	delete this->right;
}

IfElse::IfElse(Base* cond, Base* then, Base* other)
{
	this->type = NODE_IF;
	
	this->cond  = cond;
	this->then  = then;
	this->other = other;
}
IfElse::~IfElse()
{
	delete this->cond;
	delete this->then;
	delete this->other;
}

Case::Case()
{
	this->then = NULL;
}
Case::~Case()
{
	for (auto node = this->checks.begin(); node != this->checks.end(); ++node)
	{
		delete *node;
	}
	this->checks.clear();
	delete this->then;
}
Match::Match()
{
	this->type = NODE_MATCH;
	
	this->comp  = NULL;
	this->other = NULL;
}
Match::~Match()
{
	for (auto node = this->cases.begin(); node != this->cases.end(); ++node)
	{
		delete *node;
	}
	this->cases.clear();
	delete this->comp;
	delete this->other;
}
