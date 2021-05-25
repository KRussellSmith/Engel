#ifndef node_header
#define node_header
#include "lexer.hpp"
#include <vector>
typedef enum
{
	NODE_BINARY,
	NODE_INT,
	NODE_IF,
	NODE_MATCH,
} NodeType;
namespace Node
{
	class Base
	{
	public:
		NodeType type;
	};
	class Const: public Base
	{
	public:
		const char* value;
		int length;
		Const(NodeType type, Token* value);
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
	class IfElse: public Base
	{
	public:
		Base* cond;
		Base* then;
		Base* other;
		
		IfElse(Base* cond, Base* then, Base* other);
		~IfElse();
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
}
#endif
