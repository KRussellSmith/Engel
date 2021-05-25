#ifndef parser_header
#define parser_header
#include "node.hpp"
class Parser
{
private:
	Lexer* lexer;
	Token curr;
	Token prev;
	
	bool fin();
	void advance();
	void eat(TokenType type, const char* error);
	bool taste(TokenType type);
	bool sniff(TokenType type);
	void skip_breaks();
	
	Node::Base* factor();
	Node::Base* exp();
	Node::Base* mul();
	Node::Base* add();
	Node::Base* expr();
	Node::Base* stmt();
	Node::Base* dec();
	
public:
	Parser(Lexer* lexer);
	Node::Base* parse();
};
#endif
