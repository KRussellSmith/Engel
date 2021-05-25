#ifndef node_header
#define node_header
#include "value.hpp"
#include "lexer.hpp"
#include <vector>
typedef enum
{
	NODE_EXPR,
	NODE_GROUP,
	NODE_UNARY,
	NODE_BINARY,
	NODE_CONST,
	NODE_COND,
	NODE_IF,
	NODE_WHILE,
	NODE_MATCH,
	NODE_COMP,
	NODE_DEC,
	NODE_GET,
	NODE_SET,
	NODE_INTERP,
	NODE_BLOCK,
	NODE_FUNC,
	NODE_FUNCBODY,
	NODE_RETURN,
	NODE_FUNCCALL,
	NODE_FIN,
} NodeType;
namespace Node
{
	class Base
	{
	public:
		NodeType type;
		Base(NodeType type);
	};

	// Generic nodes (i.e. cast as Base*) shall NOT be directly freed via `delete`
	// instead, this function should be called to properly cast them
	// after checking their NodeType, ensuring the correct destructor is called.
	void destroy(Base* node);

	class Const: public Base
	{
	public:
		Value value;
		Const(Token* value);
	};
	class Expr: public Base
	{
	public:
		Base* child;
		Expr(Base* child);
		~Expr();
	};
	class Group: public Base
	{
	public:
		Base* child;
		Group(Base* child);
		~Group();
	};
	class Unary: public Base
	{
	public:
		Base* child;
		TokenType op;
		Unary(TokenType op, Base* child);
		~Unary();
	};
	class Cond: public Base
	{
	public:
		TokenType op;
		Base* left;
		Base* right;
		Cond(TokenType op, Base* left, Base* right);
		~Cond();
	};
	class Binary: public Base
	{
	public:
		Base* left;
		Base* right;
		TokenType op;
		Binary(TokenType op, Base* left, Base* right);
		~Binary();
	};
	class Comparison
	{
	public:
		Base* value;
		TokenType type;
		Comparison(TokenType type, Base* value);
		~Comparison();
	};
	class Comparisons: public Base
	{
	public:
		std::vector<Comparison*> list;
		Base* primer;
		Comparisons(Base* primer);
		~Comparisons();
	};
	class Case
	{
	public:
		std::vector<Base*> checks;
		Base* then;
		Case();
		~Case();
	};
	class Match: public Base
	{
	public:
		Base* comp;
		std::vector<Case*> cases;
		Base* other;
		Match();
		~Match();
	};
	class Declaration
	{
	public:
		Token name;
		Base* value;
		bool  mut;
		Declaration(Token name, Base* value, bool mut);
		~Declaration();
	};
	class Declarations: public Base
	{
	public:
		std::vector<Declaration*> list;
		Declarations();
		~Declarations();
	};
	class Get: public Base
	{
	public:
		Token name;
		Get(Token name);
	};
	class Set: public Base
	{
	public:
		Base* left;
		Base* right;
		TokenType op;
		Set(TokenType op, Base* left, Base* right);
		~Set();
	};
	class Interpolation
	{
	public:
		Token chars;
		Base* value;
		Interpolation(Token chars, Base* value);
		~Interpolation();
	};
	class StringInterp: public Base
	{
	public:
		std::vector<Interpolation*> list;
		Token cap;
		StringInterp();
		~StringInterp();
	};
	class Block: public Base
	{
	public:
		std::vector<Base*> list;
		Block();
		~Block();
	};
	class Func: public Base
	{
	public:
		std::vector<Base*> args;
		Base* body;
		Func();
		~Func();
	};
	class FuncBody: public Base
	{
	public:
		std::vector<Base*> list;
		FuncBody();
		~FuncBody();
	};
	class Return: public Base
	{
	public:
		Base* expr;
		Return(Base* expr);
		~Return();
	};
	class FuncCall: public Base
	{
	public:
		Base* callee;
		std::vector<Base*> args;
		FuncCall(Base* callee);
		~FuncCall();
	};
	class If: public Base
	{
	public:
		Base* cond;
		Base* then;
		Base* other;
		If(Base* cond, Base* then, Base* other);
		~If();
	};
	class While: public Base
	{
	public:
		Base* cond;
		Base* then;
		Base* other;
		While(Base* cond, Base* then, Base* other);
		~While();
	};
}
#endif
