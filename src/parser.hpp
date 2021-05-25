#ifndef parser_header
#define parser_header
#include "node.hpp"
#include "vm.hpp"
#include "langs.hpp"
class Parser
{
private:
	Lexer* lexer;
	Token curr;
	Token prev;
	VM*  vm;
	
	Lang lang;

	bool errored;
	bool panic;
	void error(const char* msg[]);
	bool fin();
	void advance();
	void eat(TokenType type, const char* error[]);
	bool taste(TokenType type);
	bool sniff(TokenType type);
	bool taste(const int types[]);
	bool sniff(const int types[]);
	void skip_breaks();

	Node::Base* func_body();
	Node::Base* finish_call(Node::Base* node);
	Node::Base* call_to(Node::Base* node);

	Node::Base* factor();
	Node::Base* exp();
	Node::Base* mul();
	Node::Base* add();
	Node::Base* shift();
	Node::Base* band();
	Node::Base* xor_();
	Node::Base* bor();
	Node::Base* comp();
	Node::Base* and_();
	Node::Base* or_();
	Node::Base* coal();
	Node::Base* ifelse();
	Node::Base* ass();
	Node::Base* expr();
	Node::Base* block();
	Node::Base* stmt();
	Node::Base* dec();
	
public:
	Parser(Lexer* lexer, VM* vm, Lang lang);
	Node::Base* parse();
	bool panicking();
};
#endif
